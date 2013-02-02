# The Doomsday Engine Project: Server Shell -- Common Functionality
# Copyright (c) 2013 Jaakko Keränen <jaakko.keranen@iki.fi>
#
# This program is distributed under the GNU General Public License
# version 2 (or, at your option, any later version). Please visit
# http://www.gnu.org/licenses/gpl.html for details.

include(../config.pri)

TEMPLATE = lib
TARGET   = deng_shell
VERSION  = 0.1.0

include(../dep_deng2.pri)

win32 {
    # Keep the version number out of the file name.
    TARGET_EXT = .dll
}

DEFINES += __LIBSHELL__

INCLUDEPATH += include

# Public headers.
HEADERS += \
    include/de/shell/KeyEvent \
    include/de/shell/Link \
    include/de/shell/Protocol \
    include/de/shell/TextCanvas \
    include/de/shell/TextEditWidget \
    include/de/shell/TextRootWidget \
    include/de/shell/TextWidget \
    include/de/shell/keyevent.h \
    include/de/shell/libshell.h \
    include/de/shell/link.h \
    include/de/shell/protocol.h \
    include/de/shell/textcanvas.h \
    include/de/shell/texteditwidget.h \
    include/de/shell/textrootwidget.h \
    include/de/shell/textwidget.h

# Sources and private headers.
SOURCES += \
    src/libshell.cpp \
    src/link.cpp \
    src/protocol.cpp \
    src/textcanvas.cpp \
    src/texteditwidget.cpp \
    src/textrootwidget.cpp \
    src/textwidget.cpp

# Installation ---------------------------------------------------------------

macx {
    linkDylibToBundledLibdeng2(libdeng_shell)

    doPostLink("install_name_tool -id @executable_path/../Frameworks/libdeng_shell.0.dylib libdeng_shell.0.dylib")

    # Update the library included in the main app bundle.
    doPostLink("mkdir -p ../client/Doomsday.app/Contents/Frameworks")
    doPostLink("cp -fRp libdeng_shell*dylib ../client/Doomsday.app/Contents/Frameworks")
}
else {
    INSTALLS += target
    target.path = $$DENG_LIB_DIR
}