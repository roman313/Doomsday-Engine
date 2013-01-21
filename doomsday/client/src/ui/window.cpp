/** @file window.cpp Window manager. Window manager that manages a QWidget-based window. 
 * @ingroup base
 *
 * The Doomsday window management is responsible for the positioning, sizing,
 * and state of the game's native windows. In practice, the code operates on Qt
 * top-level windows.
 *
 * At the moment, the quality of the code is adequate at best. See the todo
 * notes below for ideas for future improvements.
 *
 * @todo Instead of 'rect' and 'normalRect', the window should have a
 * 'fullscreenSize' and a 'normalRect'. It isn't ideal that when toggling
 * between fullscreen and windowed mode, the fullscreen resolution is chosen
 * based on the size of the normal-mode window.
 *
 * @todo It is not a good idea to duplicate window state locally (position,
 * size, flags). Much of the complexity here is due to this duplication, trying
 * to keep all the state consistent. Instead, the real QWidget should be used
 * for these properties. Qt has a mechanism for storing the state of a window:
 * QWidget::saveGeometry(), QMainWindow::saveState().
 *
 * @todo Refactor for multiple window support. One window should be the "main"
 * window while others are secondary windows.
 *
 * @todo Deferred window changes should be done using a queue-type solution
 * where it is possible to schedule multiple tasks into the future separately
 * for each window. Each window could have its own queue.
 *
 * @todo Platform-specific behavior should be encapsulated in subclasses, e.g.,
 * MacWindowBehavior. This would make the code easier to follow and more adaptable
 * to the quirks of each platform.
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2008 Jamie Jones <jamie_jones_au@yahoo.com.au>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA</small>
 */

#include <QWidget>
#include <QApplication>
#include <QPaintEvent>
#include <QDesktopWidget>
#include <QDebug>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <de/App>
#include <de/Config>
#include <de/Record>
#include <de/NumberValue>
#include <de/ArrayValue>

#ifdef MACOSX
#  include "ui/displaymode_native.h"
#endif

#include "de_platform.h"

#include "ui/window.h"
#include "ui/consolewindow.h"
#include "ui/canvaswindow.h"
#include "ui/displaymode.h"
#include "../updater/downloaddialog.h"
#include "sys_system.h"
#include "busymode.h"
#include "dd_main.h"
#include "con_main.h"
#ifdef __CLIENT__
#  include "gl/gl_main.h"
#endif
#include "ui/ui_main.h"
#include "filesys/fs_util.h"

#include <de/c_wrapper.h>
#include <de/Log>

#ifdef MACOSX
static const int WAIT_MILLISECS_AFTER_MODE_CHANGE = 100; // ms
#else
static const int WAIT_MILLISECS_AFTER_MODE_CHANGE = 10; // ms
#endif

/// Used to determine the valid region for windows on the desktop.
/// A window should never go fully (or nearly fully) outside the desktop.
static const int DESKTOP_EDGE_GRACE = 30; // pixels

static QRect desktopRect()
{
    /// @todo Multimonitor? This checks the default screen.
    return QApplication::desktop()->screenGeometry();
}

/*
static QRect desktopValidRect()
{
    return desktopRect().adjusted(DESKTOP_EDGE_GRACE, DESKTOP_EDGE_GRACE,
                                  -DESKTOP_EDGE_GRACE, -DESKTOP_EDGE_GRACE);
}
*/

static void updateMainWindowLayout(void);
static void updateWindowStateAfterUserChange(void);
static void useAppliedGeometryForWindows(void);
static void notifyAboutModeChange(void);
static void endWindowWait(void);

struct ddwindow_s
{
    CanvasWindow* widget;   ///< The widget this window represents.
    void (*drawFunc)(void); ///< Draws the contents of the canvas.
    QRect appliedGeometry;  ///< Saved for detecting when changes have occurred.
    bool needShowFullscreen;
    bool needReshowFullscreen;
    bool needShowNormal;
    bool needRecreateCanvas;
    bool needWait;
    bool willUpdateWindowState;

    ddwindowtype_t type;
    boolean inited;
    RectRaw geometry;       ///< Current actual geometry.
    RectRaw normalGeometry; ///< Normal-mode geometry (when not maximized or fullscreen).
    int colorDepthBits;
    int flags;
    consolewindow_t console; ///< Only used for WT_CONSOLE windows.

    void __inline assertWindow() const
    {
        assert(this);
        assert(widget);
    }

    bool isBeingAdjusted() const {
        return needShowFullscreen || needReshowFullscreen || needShowNormal || needRecreateCanvas || needWait;
    }

    int x() const      { return geometry.origin.x; }
    int y() const      { return geometry.origin.y; }
    int width() const  { return geometry.size.width; }
    int height() const { return geometry.size.height; }
    QRect rect() const { return QRect(x(), y(), width(), height()); }

    QRect normalRect() const { return QRect(normalGeometry.origin.x,
                                            normalGeometry.origin.y,
                                            normalGeometry.size.width,
                                            normalGeometry.size.height); }

    bool checkFlag(int flag) const { return (flags & flag) != 0; }

    /**
     * Checks all command line options that affect window geometry and applies
     * them to this Window.
     */
    void modifyAccordingToOptions()
    {
        if(CommandLine_Exists("-nofullscreen") || CommandLine_Exists("-window"))
        {
            setFlag(DDWF_FULLSCREEN, false);
        }

        if(CommandLine_Exists("-fullscreen") || CommandLine_Exists("-nowindow"))
        {
            setFlag(DDWF_FULLSCREEN);
        }

        if(CommandLine_CheckWith("-width", 1))
        {
            geometry.size.width = qMax(WINDOW_MIN_WIDTH, atoi(CommandLine_Next()));
            if(!(flags & DDWF_FULLSCREEN))
            {
                normalGeometry.size.width = geometry.size.width;
            }
        }

        if(CommandLine_CheckWith("-height", 1))
        {
            geometry.size.height = qMax(WINDOW_MIN_HEIGHT, atoi(CommandLine_Next()));
            if(!(flags & DDWF_FULLSCREEN))
            {
                normalGeometry.size.height = geometry.size.height;
            }
        }

        if(CommandLine_CheckWith("-winsize", 2))
        {
            geometry.size.width  = qMax(WINDOW_MIN_WIDTH,  atoi(CommandLine_Next()));
            geometry.size.height = qMax(WINDOW_MIN_HEIGHT, atoi(CommandLine_Next()));

            if(!(flags & DDWF_FULLSCREEN))
            {
                normalGeometry.size.width  = geometry.size.width;
                normalGeometry.size.height = geometry.size.height;
            }
        }

        if(CommandLine_CheckWith("-colordepth", 1) || CommandLine_CheckWith("-bpp", 1))
        {
            colorDepthBits = qBound(8, atoi(CommandLine_Next()), 32);
        }

        if(CommandLine_Check("-nocenter"))
        {
            setFlag(DDWF_CENTER, false);
        }

        if(CommandLine_CheckWith("-xpos", 1))
        {
            normalGeometry.origin.x = atoi(CommandLine_Next());
            setFlag(DDWF_CENTER | DDWF_MAXIMIZE, false);
        }

        if(CommandLine_CheckWith("-ypos", 1))
        {
            normalGeometry.origin.y = atoi(CommandLine_Next());
            setFlag(DDWF_CENTER | DDWF_MAXIMIZE, false);
        }

        if(CommandLine_Check("-center"))
        {
            setFlag(DDWF_CENTER);
        }

        if(CommandLine_Check("-maximize"))
        {
            setFlag(DDWF_MAXIMIZE);
        }

        if(CommandLine_Check("-nomaximize"))
        {
            setFlag(DDWF_MAXIMIZE, false);
        }
    }

    bool applyDisplayMode()
    {
        assertWindow();

        if(!DisplayMode_Count()) return true; // No modes to change to.

        if(flags & DDWF_FULLSCREEN)
        {
            const DisplayMode* mode = DisplayMode_FindClosest(width(), height(), colorDepthBits, 0);
            if(mode && DisplayMode_Change(mode, true /* fullscreen: capture */))
            {
                geometry.size.width = DisplayMode_Current()->width;
                geometry.size.height = DisplayMode_Current()->height;
#if defined MACOSX && defined __CLIENT__
                // Pull the window again over the shield after the mode change.
                DisplayMode_Native_Raise(Window_NativeHandle(this));
#endif
                Window_TrapMouse(this, true);
                return true;
            }
        }
        else
        {
            return DisplayMode_Change(DisplayMode_OriginalMode(),
                                      false /* windowed: don't capture */);
        }
        return false;
    }

    QRect centeredGeometry() const
    {
        QSize winSize = normalRect().size();

        // Center the window.
        QSize screenSize = desktopRect().size();
        LOG_DEBUG("centeredGeometry: Current desktop rect %ix%i") << screenSize.width() << screenSize.height();
        return QRect(desktopRect().topLeft() +
                     QPoint((screenSize.width()  - winSize.width())  / 2,
                            (screenSize.height() - winSize.height()) / 2),
                     winSize);
    }

    /**
     * Applies the information stored in the Window instance to the actual
     * widget geometry. Centering is applied in this stage (it only affects the
     * widget's geometry).
     */
    void applyWindowGeometry()
    {
        LOG_AS("applyWindowGeometry");

        assertWindow();

        // While we're adjusting the window, the window move/resizing callbacks
        // should've mess with the geometry values.
        needWait = true;
        LegacyCore_Timer(WAIT_MILLISECS_AFTER_MODE_CHANGE * 20, endWindowWait);

        bool modeChanged = applyDisplayMode();

        if(modeChanged)
        {
            // Others might be interested to hear about the mode change.
            LegacyCore_Timer(WAIT_MILLISECS_AFTER_MODE_CHANGE, notifyAboutModeChange);
        }

        /*
         * The following is a bit convoluted. The core idea is this, though: on
         * some platforms, changes to the window's mode (normal, maximized,
         * fullscreen/frameless) do not occur immediately. Instead, control
         * needs to return to the event loop and the native window events need
         * to play out. Thus some of the operations have to be performed in a
         * deferred way, after a short wait. The ideal would be to listen to
         * the native events and trigger the necessary updates after they
         * occur; however, now we just use naive time-based delays.
         */

        if(flags & DDWF_FULLSCREEN)
        {
            LOG_DEBUG("fullscreen mode (mode changed? %b)") << modeChanged;

            if(!modeChanged) return; // We don't need to do anything.

            if(widget->isVisible())
            {
                needShowFullscreen = !widget->isFullScreen();

#if defined(WIN32) || defined(Q_WS_X11)
                if(widget->isFullScreen())
                {
                    needShowFullscreen = false;
                    needReshowFullscreen = true;
                }
#endif
                LOG_DEBUG("widget is visible, need showFS:%b reshowFS:%b")
                        << needShowFullscreen << needReshowFullscreen;

#ifdef MACOSX
                // Kludge! See updateMainWindowLayout().
                appliedGeometry = QRect(0, 0,
                                        DisplayMode_Current()->width,
                                        DisplayMode_Current()->height);
#endif

                // The window is already visible, so let's allow a mode change to resolve itself
                // before we go changing the window.
                LegacyCore_Timer(WAIT_MILLISECS_AFTER_MODE_CHANGE, updateMainWindowLayout);
            }
            else
            {
                LOG_DEBUG("widget is not visible, setting geometry to ")
                        << DisplayMode_Current()->width << "x"
                        << DisplayMode_Current()->height;
                widget->setGeometry(0, 0, DisplayMode_Current()->width, DisplayMode_Current()->height);
            }
        }
        else
        {
            // The window is in windowed mode (frames and window decoration visible).
            // We will restore it to its previous position and size.
            QRect geom = normalRect(); // Previously stored normal geometry.

            if(flags & DDWF_CENTER)
            {
                // Center the window.
                geom = centeredGeometry();
            }

            if(flags & DDWF_MAXIMIZE)
            {
                // When a window is maximized, we'll let the native WM handle the sizing.
                if(widget->isVisible())
                {
                    LOG_DEBUG("Window maximized.");
                    widget->showMaximized();
                }

                // Non-visible windows will be shown later
                // (as maximized, if the flag is set).
            }
            else
            {
                // The window is in normal mode: not maximized or fullscreen.

                // If the window is already visible, changes to it need to be deferred
                // so that the native counterpart can be updated, too
                if(widget->isVisible() && (modeChanged || widget->isMaximized()))
                {
                    if(modeChanged)
                    {
                        // We'll wait before the mode change takes full effect.
                        needShowNormal = true;
                        LegacyCore_Timer(WAIT_MILLISECS_AFTER_MODE_CHANGE, updateMainWindowLayout);
                    }
                    else
                    {
                        // Display mode was not changed, so we can immediately
                        // change the window state.
                        widget->showNormal();
                    }
                }

                appliedGeometry = geom;

                if(widget->isVisible())
                {
                    // The native window may not be ready to receive the updated geometry
                    // (e.g., window decoration not made visible yet). We'll apply the
                    // geometry after a delay.
                    LegacyCore_Timer(50 + WAIT_MILLISECS_AFTER_MODE_CHANGE, useAppliedGeometryForWindows);
                }
                else
                {
                    // The native window is not visible yet, so we can apply any number
                    // of changes we like.
                    widget->setGeometry(geom);
                }
            }
        }
    }

    /**
     * Retrieves the actual widget geometry and updates the information stored
     * in the Window instance.
     */
    void fetchWindowGeometry()
    {
        assertWindow();

        setFlag(DDWF_MAXIMIZE, widget->isMaximized());

        QRect rect = widget->geometry();
        geometry.origin.x = rect.x();
        geometry.origin.y = rect.y();
        geometry.size.width = rect.width();
        geometry.size.height = rect.height();

        // If the window is presently maximized or fullscreen, we will not
        // store the actual coordinates.
        if(!widget->isMaximized() && !(flags & DDWF_FULLSCREEN) && !isBeingAdjusted())
        {
            normalGeometry.origin.x = rect.x();
            normalGeometry.origin.y = rect.y();
            normalGeometry.size.width = rect.width();
            DEBUG_Message(("ngw=%i [A]\n", normalGeometry.size.width));
            normalGeometry.size.height = rect.height();
        }

        LOG_DEBUG("Current window geometry: %i,%i %ix%i (max:%b)")
                << geometry.origin.x << geometry.origin.y
                << geometry.size.width << geometry.size.height
                << ((flags & DDWF_MAXIMIZE) != 0);
        LOG_DEBUG("Normal window geometry:  %i,%i %ix%i")
                << normalGeometry.origin.x << normalGeometry.origin.y
                << normalGeometry.size.width << normalGeometry.size.height;
    }

    void setFlag(int flag, bool set = true)
    {
        if(set)
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
            if(flag & DDWF_CENTER) LOG_DEBUG("Clearing DDWF_CENTER");
        }
    }

    bool applyAttributes(int* attribs)
    {
#ifdef __CLIENT__
        LOG_AS("applyAttributes");

        bool changed = false;

        // Parse the attributes array and check the values.
        assert(attribs);
        for(int i = 0; attribs[i]; ++i)
        {
            switch(attribs[i++])
            {
            case DDWA_X:
                if(x() != attribs[i])
                {
                    normalGeometry.origin.x = attribs[i];
                    changed = true;
                }
                break;
            case DDWA_Y:
                if(y() != attribs[i])
                {
                    normalGeometry.origin.y = attribs[i];
                    changed = true;
                }
                break;
            case DDWA_WIDTH:
                if(width() != attribs[i])
                {
                    if(attribs[i] < WINDOW_MIN_WIDTH) return false;
                    normalGeometry.size.width = geometry.size.width = attribs[i];
                    DEBUG_Message(("ngw=%i [B]\n", normalGeometry.size.width));
                    changed = true;
                }
                break;
            case DDWA_HEIGHT:
                if(height() != attribs[i])
                {
                    if(attribs[i] < WINDOW_MIN_HEIGHT) return false;
                    normalGeometry.size.height = geometry.size.height = attribs[i];
                    changed = true;
                }
                break;
#define IS_NONZERO(x) ((x) != 0)
            case DDWA_CENTER:
                if(IS_NONZERO(attribs[i]) != IS_NONZERO(checkFlag(DDWF_CENTER)))
                {
                    setFlag(DDWF_CENTER, attribs[i]);
                    changed = true;
                }
                break;
            case DDWA_MAXIMIZE:
                if(IS_NONZERO(attribs[i]) != IS_NONZERO(checkFlag(DDWF_MAXIMIZE)))
                {
                    setFlag(DDWF_MAXIMIZE, attribs[i]);
                    changed = true;
                }
                break;
            case DDWA_FULLSCREEN:
                if(IS_NONZERO(attribs[i]) != IS_NONZERO(checkFlag(DDWF_FULLSCREEN)))
                {
                    if(attribs[i] && Updater_IsDownloadInProgress())
                    {
                        // Can't go to fullscreen when downloading.
                        return false;
                    }
                    setFlag(DDWF_FULLSCREEN, attribs[i]);
                    changed = true;
                }
                break;
            case DDWA_VISIBLE:
                if(IS_NONZERO(attribs[i]) != IS_NONZERO(checkFlag(DDWF_VISIBLE)))
                {
                    setFlag(DDWF_VISIBLE, attribs[i]);
                    changed = true;
                }
                break;
#undef IS_NONZERO
            case DDWA_COLOR_DEPTH_BITS:
                qDebug() << attribs[i] << colorDepthBits;
                if(attribs[i] != colorDepthBits)
                {
                    colorDepthBits = attribs[i];
                    if(colorDepthBits < 8 || colorDepthBits > 32) return false; // Illegal value.
                    changed = true;
                }
                break;
            default:
                // Unknown attribute.
                return false;
            }
        }

        if(!changed)
        {
            VERBOSE(Con_Message("New window attributes same as before.\n"));
            return true;
        }

        // Seems ok, apply them.
        applyWindowGeometry();
#endif // __CLIENT__
        return true;
    }

    void updateLayout()
    {
        //qDebug() << "Window::updateLayout:" << widget->geometry() << widget->canvas().geometry();

        setFlag(DDWF_MAXIMIZE, widget->isMaximized());

        geometry.size.width = widget->width();
        geometry.size.height = widget->height();

        if(!(flags & DDWF_FULLSCREEN))
        {
            DEBUG_Message(("Updating current view geometry for window, fetched %i x %i.\n", width(), height()));

            if(!(flags & DDWF_MAXIMIZE) && !isBeingAdjusted())
            {
                // Update the normal-mode geometry (not fullscreen, not maximized).
                normalGeometry.size.width = geometry.size.width;
                DEBUG_Message(("ngw=%i [C]\n", normalGeometry.size.width));
                normalGeometry.size.height = geometry.size.height;

                DEBUG_Message(("Updating normal view geometry for window, fetched %i x %i.\n", width(), height()));
            }
        }
        else
        {
            DEBUG_Message(("Updating view geometry for fullscreen (%i x %i).\n", width(), height()));
        }

#ifdef __CLIENT__
        // Update viewports.
        R_SetViewGrid(0, 0);
        if(BusyMode_Active() || UI_IsActive() || !DD_GameLoaded())
        {
            // Update for busy mode.
            R_UseViewPort(0);
        }
        R_LoadSystemFonts();
        if(UI_IsActive())
        {
            UI_UpdatePageLayout();
        }
#endif
    }
};

/// Current active window where all drawing operations occur.
const Window* theWindow;

static boolean winManagerInited = false;

static Window mainWindow;
static boolean mainWindowInited = false;

static void updateMainWindowLayout(void)
{
    Window* win = Window_Main();

    if(win->needReshowFullscreen)
    {
        LOG_DEBUG("Main window re-set to fullscreen mode.");
        win->needReshowFullscreen = false;
        win->widget->showNormal();
        win->widget->showFullScreen();
    }

    if(win->needShowFullscreen)
    {
        LOG_DEBUG("Main window to fullscreen mode.");
        win->needShowFullscreen = false;
        win->widget->showFullScreen();
    }

    if(win->flags & DDWF_FULLSCREEN)
    {
#if defined MACOSX && defined __CLIENT__
        // For some interesting reason, we have to scale the window twice in fullscreen mode
        // or the resulting layout won't be correct.
        win->widget->setGeometry(QRect(0, 0, 320, 240));
        win->widget->setGeometry(win->appliedGeometry);

        DisplayMode_Native_Raise(Window_NativeHandle(win));
#endif
        Window_TrapMouse(win, true);
    }

    if(win->needShowNormal)
    {
        LOG_DEBUG("Main window to normal mode (center:%b).") << ((win->flags & DDWF_CENTER) != 0);
        win->needShowNormal = false;
        win->widget->showNormal();
    }
}

static void useAppliedGeometryForWindows(void)
{
    Window* win = Window_Main();
    if(!win || !win->widget) return;

    if(win->flags & DDWF_CENTER)
    {
        win->appliedGeometry = win->centeredGeometry();
    }

    DEBUG_Message(("Using applied geometry: (%i,%i) %ix%i\n",
                   win->appliedGeometry.x(),
                   win->appliedGeometry.y(),
                   win->appliedGeometry.width(),
                   win->appliedGeometry.height()));
    win->widget->setGeometry(win->appliedGeometry);
}

Window* Window_Main(void)
{
    return &mainWindow;
}

static void notifyAboutModeChange(void)
{
    LOG_MSG("Display mode has changed.");
    DENG2_APP->notifyDisplayModeChanged();
}

static void endWindowWait(void)
{
    Window* win = Window_Main();
    if(win)
    {
        DEBUG_Message(("Window is no longer waiting for geometry changes.\n"));

        // This flag is used for protecting against mode change resizings.
        win->needWait = false;
    }
}

static int getWindowIdx(const Window* wnd)
{
    /// @todo  Multiple windows.
    if(wnd == &mainWindow) return mainWindowIdx;

    return 0;
}

static __inline Window *getWindow(uint idx)
{
    if(!winManagerInited)
        return NULL; // Window manager is not initialized.

    if(idx == 1)
        return &mainWindow;

    //assert(false); // We can only have window 1 (main window).
    return NULL;
}

Window* Window_ByIndex(uint id)
{
    return getWindow(id);
}

#if 0
static boolean setDDWindow(Window *window, int newWidth, int newHeight,
                           int newBPP, uint wFlags, uint uFlags)
{
    int             width, height, bpp, flags;
    boolean         newGLContext = false;
    boolean         changeWindowDimensions = false;
    boolean         inControlPanel = false;

    if(novideo)
        return true;

    if(uFlags & DDSW_NOCHANGES)
        return true; // Nothing to do.

    // Grab the current values.
    width = window->geometry.size.width;
    height = window->geometry.size.height;
    bpp = window->bpp;
    flags = window->flags;
    // Force update on init?
    if(!window->inited && window->type == WT_NORMAL)
    {
        newGLContext = true;
    }

    if(window->type == WT_NORMAL)
        inControlPanel = UI_IsActive();

    // Change to fullscreen?
    if(!(uFlags & DDSW_NOFULLSCREEN) &&
       (flags & DDWF_FULLSCREEN) != (wFlags & DDWF_FULLSCREEN))
    {
        flags ^= DDWF_FULLSCREEN;

        if(window->type == WT_NORMAL)
        {
            newGLContext = true;
            //changeVideoMode = true;
        }
    }

    // Change window size?
    if(!(uFlags & DDSW_NOSIZE) && (width != newWidth || height != newHeight))
    {
        width = newWidth;
        height = newHeight;
        changeWindowDimensions = true;

        if(window->type == WT_NORMAL)
            newGLContext = true;
    }

    // Change BPP (bits per pixel)?
    if(window->type == WT_NORMAL)
    {
        if(!(uFlags & DDSW_NOBPP) && bpp != newBPP)
        {
            if(!(newBPP == 32 || newBPP == 16))
                Con_Error("Sys_SetWindow: Unsupported BPP %i.", newBPP);

            bpp = newBPP;

            newGLContext = true;
            //changeVideoMode = true;
        }
    }

    if(changeWindowDimensions && window->type == WT_NORMAL)
    {
        // Can't change the resolution while the UI is active.
        // (controls need to be repositioned).
        if(inControlPanel)
            UI_End();
    }
/*
    if(changeVideoMode)
    {
        if(flags & DDWF_FULLSCREEN)
        {
            if(!Sys_ChangeVideoMode(width, height, bpp))
            {
                Sys_CriticalMessage("Sys_SetWindow: Resolution change failed.");
                return false;
            }
        }
        else
        {
            // Go back to normal display settings.
            ChangeDisplaySettings(0, 0);
        }
    }
*/
    // Update the current values.
    window->geometry.size.width = width;
    window->geometry.size.height = height;
    window->bpp = bpp;
    window->flags = flags;
    if(!window->inited)
        window->inited = true;

    // Do NOT modify Window properties after this point.

    // Do we need a new GL context due to changes to the window?
    if(newGLContext)
    {
#if 0
        // Maybe requires a renderer restart.
        extern boolean usingFog;

        boolean         glIsInited = GL_IsInited();
#if defined(WIN32)
        void           *data = window->hWnd;
#else
        void           *data = NULL;
#endif
        boolean         hadFog = false;

        if(glIsInited)
        {
            // Shut everything down, but remember our settings.
            hadFog = usingFog;
            GL_TotalReset();

            if(DD_GameLoaded() && gx.UpdateState)
                gx.UpdateState(DD_RENDER_RESTART_PRE);

            R_UnloadSvgs();
            GL_ReleaseTextures();
        }

        if(createContext(window->geometry.size.width, window->geometry.size.height,
               window->normal.bpp, (window->flags & DDWF_FULLSCREEN)? false : true, data))
        {
            // We can get on with initializing the OGL state.
            Sys_GLConfigureDefaultState();
        }

        if(glIsInited)
        {
            // Re-initialize.
            GL_TotalRestore();
            GL_InitRefresh();

            if(hadFog)
                GL_UseFog(true);

            if(DD_GameLoaded() && gx.UpdateState)
                gx.UpdateState(DD_RENDER_RESTART_POST);
        }
#endif
    }

    // If the window dimensions have changed, update any sub-systems
    // which need to respond.
    if(changeWindowDimensions && window->type == WT_NORMAL)
    {
        // Update viewport coordinates.
        R_SetViewGrid(0, 0);

        if(inControlPanel) // Reactivate the panel?
            Con_Execute(CMDS_DDAY, "panel", true, false);
    }

    return true;
}
#endif

/**
 * Initialize the window manager.
 * Tasks include; checking the system environment for feature enumeration.
 *
 * @return              @c true, if initialization was successful.
 */
boolean Sys_InitWindowManager(void)
{
    LOG_AS("Sys_InitWindowManager");

    if(winManagerInited)
        return true; // Already been here.

    LOG_MSG("Using Qt window management.");

    CanvasWindow::setDefaultGLFormat();

    memset(&mainWindow, 0, sizeof(mainWindow));
    winManagerInited = true;
    theWindow = &mainWindow;
    return true;
}

/**
 * Shutdown the window manager.
 *
 * @return              @c true, if shutdown was successful.
 */
boolean Sys_ShutdownWindowManager(void)
{
    if(!winManagerInited)
        return false; // Window manager is not initialized.

    /// @todo Delete all windows, not just the main one.

    // Get rid of the windows.
    Window_Delete(Window_Main());

    // Now off-line, no more window management will be possible.
    winManagerInited = false;

    return true;
}

#if 0
/**
 * Attempt to acquire a device context for OpenGL rendering and then init.
 *
 * @param width         Width of the OGL window.
 * @param height        Height of the OGL window.
 * @param bpp           0= the current display color depth is used.
 * @param windowed      @c true = windowed mode ELSE fullscreen.
 * @param data          Ptr to system-specific data, e.g a window handle
 *                      or similar.
 *
 * @return              @c true if successful.
 */
static boolean createContext(void)
{
    Con_Message("createContext: OpenGL.\n");

#if 0
    // Set GL attributes.  We want at least 5 bits per color and a 16
    // bit depth buffer.  Plus double buffering, of course.
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

    /*
    // Attempt to set the video mode.
    if(!Sys_ChangeVideoMode(Window_Width(theWindow),
                            Window_Height(theWindow),
                            Window_ColorDepthBits(theWindow)))
    {
        Con_Error("createContext: Video mode change failed.\n");
        return false;
    }
    */

    Sys_GLConfigureDefaultState();

#ifdef MACOSX
    // Vertical sync is a GL context property.
    GL_SetVSync(true);
#endif

    return true;
}
#endif

static Window* canvasToWindow(Canvas& DENG_DEBUG_ONLY(canvas))
{
    assert(mainWindow.widget->ownsCanvas(&canvas)); /// @todo multiwindow

    return &mainWindow;
}

static void drawCanvasWithCallback(Canvas& canvas)
{
    Window* win = canvasToWindow(canvas);
    if(win->drawFunc)
    {
        win->drawFunc();
    }
    // Now we can continue with the main loop (if it was paused).
    LegacyCore_ResumeLoop();
}

static void windowFocusChanged(Canvas& canvas, bool focus)
{
    Window* wnd = canvasToWindow(canvas);
    wnd->assertWindow();

    LOG_DEBUG("windowFocusChanged focus:%b fullscreen:%b hidden:%b minimized:%b")
            << focus << Window_IsFullscreen(wnd)
            << wnd->widget->isHidden() << wnd->widget->isMinimized();

    if(!focus)
    {
        DD_ClearEvents();
        I_ResetAllDevices();
        Window_TrapMouse(wnd, false);
    }
    else if(Window_IsFullscreen(wnd))
    {
        // Trap the mouse again in fullscreen mode.
        Window_TrapMouse(wnd, true);
    }

    // Generate an event about this.
    ddevent_t ev;
    ev.type = E_FOCUS;
    ev.focus.gained = focus;
    ev.focus.inWindow = getWindowIdx(wnd);
    DD_PostEvent(&ev);

#if 0
    if(Window_IsFullscreen(wnd))
    {
        if(!focus)
        {
            DisplayMode_Change(DisplayMode_OriginalMode());
        }
        else
        {
            wnd->applyDisplayMode();
        }
    }
#endif
}

static void finishMainWindowInit(Canvas& canvas)
{
    Window* win = canvasToWindow(canvas);
    assert(win == &mainWindow);

#if defined MACOSX && defined __CLIENT__
    if(Window_IsFullscreen(win))
    {
        // The window must be manually raised above the shielding window put up by
        // the display capture.
        DisplayMode_Native_Raise(Window_NativeHandle(win));
    }
#endif

    win->widget->raise();
    win->widget->activateWindow();

    // Automatically grab the mouse from the get-go if in fullscreen mode.
    if(Mouse_IsPresent() && Window_IsFullscreen(win))
    {
        Window_TrapMouse(&mainWindow, true);
    }

    win->widget->canvas().setFocusFunc(windowFocusChanged);

#ifdef WIN32
    if(Window_IsFullscreen(win))
    {
        // It would seem we must manually give our canvas focus. Bug in Qt?
        win->widget->canvas().setFocus();
    }
#endif

    DD_FinishInitializationAfterWindowReady();
}

static bool windowIsClosing(CanvasWindow&)
{
    LOG_DEBUG("Window is about to close, executing 'quit'.");

    /// @todo autosave and quit?
    Con_Execute(CMDS_DDAY, "quit", true, false);

    // We are not authorizing immediate closing of the window;
    // engine shutdown will take care of it later.
    return false; // don't close
}

/**
 * See the todo notes. Duplicating state is not a good idea.
 */
static void updateWindowStateAfterUserChange()
{
    Window* win = Window_Main();
    if(!win || !win->widget) return;

    win->fetchWindowGeometry();
    win->willUpdateWindowState = false;
}

static void windowWasMoved(CanvasWindow& cw)
{
    LOG_AS("windowWasMoved");

    Window* win = canvasToWindow(cw.canvas());
    DENG_ASSERT(win);

    if(!(win->flags & DDWF_FULLSCREEN) && !win->needWait)
    {
        // The window was moved from its initial position; it is therefore
        // not centered any more (most likely).
        win->setFlag(DDWF_CENTER, false);
    }

    if(!win->willUpdateWindowState)
    {
        win->willUpdateWindowState = true;
        LegacyCore_Timer(500, updateWindowStateAfterUserChange);
    }
}

static void windowWasResized(Canvas& canvas)
{
    Window* win = canvasToWindow(canvas);
    win->assertWindow();
    win->updateLayout();
}

static Window* createWindow(ddwindowtype_t type, const char* title)
{
    if(mainWindowInited) return NULL; /// @todo  Allow multiple.

    Window* wnd = &mainWindow;
    memset(wnd, 0, sizeof(*wnd));
    mainWindowIdx = 1;

    if(type == WT_CONSOLE)
    {
        mainWindow.type = WT_CONSOLE;
        Sys_ConInit(title);
    }
    else
    {
        Window_RestoreState(&mainWindow);

        // Create the main window (hidden).
        mainWindow.widget = new CanvasWindow;
        Window_SetTitle(&mainWindow, title);

        // Minimum possible size when resizing.
        mainWindow.widget->setMinimumSize(QSize(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT));

        // After the main window is created, we can finish with the engine init.
        mainWindow.widget->canvas().setInitFunc(finishMainWindowInit);

        mainWindow.widget->setCloseFunc(windowIsClosing);
        mainWindow.widget->setMoveFunc(windowWasMoved);
        mainWindow.widget->canvas().setResizedFunc(windowWasResized);

        // Let's see if there are command line options overriding the previous state.
        mainWindow.modifyAccordingToOptions();

        // Make it so. (Not shown yet.)
        mainWindow.applyWindowGeometry();

#ifdef WIN32
        // Set an icon for the window.
        AutoStr* iconPath = AutoStr_FromText("data\\graphics\\doomsday.ico");
        F_PrependBasePath(iconPath, iconPath);
        LOG_DEBUG("Window icon: ") << de::NativePath(Str_Text(iconPath)).pretty();
        mainWindow.widget->setWindowIcon(QIcon(Str_Text(iconPath)));
#endif

        mainWindow.inited = true;
    }

    /// @todo Refactor for multiwindow support.
    mainWindowInited = true;
    return &mainWindow;
}

Window* Window_New(ddwindowtype_t type, const char* title)
{
    if(!winManagerInited) return 0;
    return createWindow(type, title);
}

void Window_Delete(Window* wnd)
{
    assert(wnd);

    if(wnd->type == WT_CONSOLE)
    {
        Sys_ConShutdown(wnd);
    }
    else
    {
        wnd->assertWindow();
        wnd->widget->canvas().setFocusFunc(0);
        wnd->widget->canvas().setResizedFunc(0);

        // Make sure we'll remember the config.
        Window_SaveState(wnd);

        if(wnd == &mainWindow)
        {
            DisplayMode_Shutdown();
        }

        // Delete the CanvasWindow.
        delete wnd->widget;

        memset(wnd, 0, sizeof(*wnd));
    }
}

boolean Window_ChangeAttributes(Window* wnd, int* attribs)
{
    Window oldState = *wnd;

    if(!wnd->applyAttributes(attribs))
    {
        // These weren't good!
        *wnd = oldState;
        return false;
    }

    // Everything ok!
    return true;
}

void Window_SwapBuffers(const Window* win)
{
    LIBDENG_ASSERT_IN_MAIN_THREAD();

    assert(win);
    if(!win->widget) return;

    // Force a swapbuffers right now.
    win->widget->canvas().swapBuffers();
}

DGLuint Window_GrabAsTexture(const Window* win, boolean halfSized)
{
    LIBDENG_ASSERT_IN_MAIN_THREAD();
    win->assertWindow();
    return win->widget->canvas().grabAsTexture(halfSized? QSize(win->width()/2, win->height()/2) : QSize());
}

boolean Window_GrabToFile(const Window* win, const char* fileName)
{
    LIBDENG_ASSERT_IN_MAIN_THREAD();
    win->assertWindow();
    return win->widget->canvas().grabImage().save(fileName);
}

void Window_Grab(const Window* win, image_t* image)
{
    Window_Grab2(win, image, false /* fullsize */);
}

void Window_Grab2(const Window* win, image_t* image, boolean halfSized)
{
    LIBDENG_ASSERT_IN_MAIN_THREAD();
    win->assertWindow();

    win->widget->canvas().grab(image, halfSized? QSize(win->width()/2, win->height()/2) : QSize());
}

void Window_SetTitle(const Window* win, const char *title)
{
    assert(win);

    LIBDENG_ASSERT_IN_MAIN_THREAD();

    switch(win->type)
    {
    case WT_NORMAL:
        assert(win->widget);
        win->widget->setWindowTitle(QString::fromLatin1(title));
        break;

    case WT_CONSOLE:
        ConsoleWindow_SetTitle(win, title);
        break;

    default:
        break;
    }
}

boolean Window_IsFullscreen(const Window* wnd)
{
    assert(wnd);
    if(wnd->type == WT_CONSOLE) return false;
    return (wnd->flags & DDWF_FULLSCREEN) != 0;
}

boolean Window_IsCentered(const Window* wnd)
{
    assert(wnd);
    if(wnd->type == WT_CONSOLE) return false;
    return (wnd->flags & DDWF_CENTER) != 0;
}

boolean Window_IsMaximized(const Window* wnd)
{
    assert(wnd);
    if(wnd->type == WT_CONSOLE) return false;
    return (wnd->flags & DDWF_MAXIMIZE) != 0;
}

void* Window_NativeHandle(const Window* wnd)
{
    if(!wnd) return 0;

#ifdef WIN32
    if(wnd->type == WT_CONSOLE)
    {
        return reinterpret_cast<void*>(wnd->console.hWnd);
    }
#endif

    if(!wnd->widget) return 0;
    return reinterpret_cast<void*>(wnd->widget->winId());
}

void Window_SetDrawFunc(Window* win, void (*drawFunc)(void))
{
    if(win->type == WT_CONSOLE) return;

    assert(win);
    assert(win->widget);

    win->drawFunc = drawFunc;
    win->widget->canvas().setDrawFunc(drawFunc? drawCanvasWithCallback : 0);
    win->widget->update();
}

void Window_Draw(Window* win)
{
    if(win->type == WT_CONSOLE) return;

#ifdef __CLIENT__

    assert(win);
    assert(win->widget);

    // The canvas needs to be recreated when the GL format has changed
    // (e.g., multisampling).
    if(win->needRecreateCanvas)
    {
        win->needRecreateCanvas = false;
        win->widget->recreateCanvas();
        return;
    }

    if(Window_ShouldRepaintManually(win))
    {
        LIBDENG_ASSERT_GL_CONTEXT_ACTIVE();

        // Perform the drawing manually right away.
        win->widget->canvas().updateGL();
    }
    else
    {
        // Don't run the main loop until after the paint event has been dealt with.
        LegacyCore_PauseLoop();

        // Request update at the earliest convenience.
        win->widget->canvas().update();
    }

#endif // __CLIENT__
}

void Window_Show(Window *wnd, boolean show)
{
    assert(wnd);

    if(wnd->type == WT_CONSOLE)
    {
        if(show)
        {
            /// @todo  Kludge: finish init in dedicated mode.
            /// This should only be done once, at startup.
            DD_FinishInitializationAfterWindowReady();
            return;
        }
    }

    assert(wnd->widget);
    if(show)
    {
        if(wnd->flags & DDWF_FULLSCREEN)
            wnd->widget->showFullScreen();
        else if(wnd->flags & DDWF_MAXIMIZE)
            wnd->widget->showMaximized();
        else
            wnd->widget->showNormal();

        //qDebug() << "Window_Show: Geometry" << wnd->widget->geometry();
    }
    else
    {
        wnd->widget->hide();
    }
}

ddwindowtype_t Window_Type(const Window* wnd)
{
    assert(wnd);
    return wnd->type;
}

struct consolewindow_s* Window_Console(Window* wnd)
{
    if(!wnd) return 0;
    return &wnd->console;
}

const struct consolewindow_s* Window_ConsoleConst(const Window* wnd)
{
    assert(wnd);
    return &wnd->console;
}

int Window_X(const Window* wnd)
{
    assert(wnd);
    return wnd->x();
}

int Window_Y(const Window* wnd)
{
    assert(wnd);
    return wnd->y();
}

int Window_Width(const Window* wnd)
{
    assert(wnd);
    return wnd->width();
}

int Window_Height(const Window *wnd)
{
    assert(wnd);
    return wnd->height();
}

int Window_NormalX(const Window* wnd)
{
    DENG_ASSERT(wnd);
    return wnd->normalRect().x();
}

int Window_NormalY(const Window* wnd)
{
    DENG_ASSERT(wnd);
    return wnd->normalRect().y();
}

int Window_NormalWidth(const Window* wnd)
{
    DENG_ASSERT(wnd);
    return wnd->normalRect().width();
}

int Window_NormalHeight(const Window* wnd)
{
    DENG_ASSERT(wnd);
    return wnd->normalRect().height();
}

int Window_ColorDepthBits(const Window* wnd)
{
    assert(wnd);
    return wnd->colorDepthBits;
}

const Size2Raw* Window_Size(const Window* wnd)
{
    assert(wnd);
    return &wnd->geometry.size;
}

/*
static QString settingsKey(uint idx, const char* name)
{
    return QString("window/%1/").arg(idx) + name;
}
*/

void Window_SaveState(Window* wnd)
{
    assert(wnd);

    // Console windows are not saved.
    if(wnd->type == WT_CONSOLE) return;

    //uint idx = mainWindowIdx;
    //DENG_ASSERT(idx == 1);

    DENG_ASSERT(wnd == &mainWindow); /// @todo  Figure out the window index if there are many.

    de::Config &config = de::App::config();

    QRect rect = wnd->rect();
    de::ArrayValue *array = new de::ArrayValue;
    *array << de::NumberValue(rect.left())
           << de::NumberValue(rect.top())
           << de::NumberValue(rect.width())
           << de::NumberValue(rect.height());
    config.names()["window.main.rect"] = array;

    QRect normRect = wnd->normalRect();
    array = new de::ArrayValue;
    *array << de::NumberValue(normRect.left())
           << de::NumberValue(normRect.top())
           << de::NumberValue(normRect.width())
           << de::NumberValue(normRect.height());
    config.names()["window.main.normalRect"] = array;

    config.names()["window.main.center"] = new de::NumberValue((wnd->flags & DDWF_CENTER) != 0);
    config.names()["window.main.maximize"] = new de::NumberValue((wnd->flags & DDWF_MAXIMIZE) != 0);
    config.names()["window.main.fullscreen"] = new de::NumberValue((wnd->flags & DDWF_FULLSCREEN) != 0);
    config.names()["window.main.colorDepth"] = new de::NumberValue(Window_ColorDepthBits(wnd));
}

void Window_RestoreState(Window* wnd)
{
    LOG_AS("Window_RestoreState");
    DENG_ASSERT(wnd);

    // Console windows can not be restored.
    if(wnd->type == WT_CONSOLE) return;

    assert(wnd == &mainWindow);  /// @todo  Figure out the window index if there are many.
    //uint idx = mainWindowIdx;
    //assert(idx == 1);

    de::Config &config = de::App::config();

    // The default state of the window is determined by these values.
    de::ArrayValue &rect = config.geta("window.main.rect");
    if(rect.size() >= 4)
    {
        QRect geom(rect.at(0).asNumber(), rect.at(1).asNumber(),
                   rect.at(2).asNumber(), rect.at(3).asNumber());
        wnd->geometry.origin.x = geom.x();
        wnd->geometry.origin.y = geom.y();
        wnd->geometry.size.width = geom.width();
        wnd->geometry.size.height = geom.height();
    }

    de::ArrayValue &normalRect = config.geta("window.main.normalRect");
    if(normalRect.size() >= 4)
    {
        QRect geom(normalRect.at(0).asNumber(), normalRect.at(1).asNumber(),
                   normalRect.at(2).asNumber(), normalRect.at(3).asNumber());
        wnd->normalGeometry.origin.x = geom.x();
        wnd->normalGeometry.origin.y = geom.y();
        wnd->normalGeometry.size.width = geom.width();
        wnd->normalGeometry.size.height = geom.height();
    }

    wnd->colorDepthBits = config.geti("window.main.colorDepth");
    wnd->setFlag(DDWF_CENTER, config.getb("window.main.center"));
    wnd->setFlag(DDWF_MAXIMIZE, config.getb("window.main.maximize"));
    wnd->setFlag(DDWF_FULLSCREEN, config.getb("window.main.fullscreen"));
}

void Window_TrapMouse(const Window* wnd, boolean enable)
{
    if(!wnd || novideo) return;

    wnd->assertWindow();
    wnd->widget->canvas().trapMouse(enable);
}

boolean Window_IsMouseTrapped(const Window* wnd)
{
    if(wnd->type == WT_CONSOLE) return false;
    wnd->assertWindow();
    return wnd->widget->canvas().isMouseTrapped();
}

boolean Window_ShouldRepaintManually(const Window* wnd)
{
    // When the pointer is not grabbed, allow the system to regulate window
    // updates (e.g., for window manipulation).
    if(Window_IsFullscreen(wnd)) return true;
    return !Mouse_IsPresent() || Window_IsMouseTrapped(wnd);
}

void Window_UpdateCanvasFormat(Window* wnd)
{
    assert(wnd != 0);
    wnd->needRecreateCanvas = true;

    // Save the relevant format settings.
    de::App::config().names()["window.fsaa"] = new de::NumberValue(Con_GetByte("vid-fsaa") != 0);
}

#if defined(UNIX) && !defined(MACOSX)
void GL_AssertContextActive(void)
{
    Window* wnd = Window_Main();
    if(wnd->type == WT_CONSOLE) return;
    assert(QGLContext::currentContext() != 0);
}
#endif

void Window_GLActivate(Window* wnd)
{
#ifdef __CLIENT__

    if(wnd->type == WT_CONSOLE) return;
    wnd->assertWindow();
    wnd->widget->canvas().makeCurrent();

    LIBDENG_ASSERT_GL_CONTEXT_ACTIVE();

#else
    DENG_UNUSED(wnd);
#endif
}

void Window_GLDone(Window* wnd)
{
#ifdef __CLIENT__

    if(wnd->type == WT_CONSOLE) return;
    wnd->assertWindow();
    wnd->widget->canvas().doneCurrent();

#else
    DENG_UNUSED(wnd);
#endif
}

QWidget* Window_Widget(Window* wnd)
{
    if(!wnd || wnd->type == WT_CONSOLE) return 0;
    return wnd->widget;
}