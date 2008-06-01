/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2007 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2008 Daniel Swanson <danij@dengine.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/**
 * p_control.c: Player Controls
 */

// HEADER FILES ------------------------------------------------------------

#include <ctype.h>

#include "de_base.h"
#include "de_play.h" // for P_LocalToConsole()
#include "de_network.h"
#include "de_misc.h"
#include "de_system.h"
#include "de_graphics.h"

#include "b_main.h"
#include "b_device.h"

// MACROS ------------------------------------------------------------------

/*
// Number of triggered impulses buffered into each player's control state
// table.  The buffer is emptied when a ticcmd is built.
#define MAX_IMPULSES 	8
#define MAX_DESCRIPTOR_LENGTH 20

#define SLOW_TURN_TIME  (6.0f / 35)
*/
// TYPES -------------------------------------------------------------------

/**
 * The control descriptors contain a mapping between symbolic control
 * names and the identifier numbers.
 */
/*
typedef struct controldesc_s {
	char    name[MAX_DESCRIPTOR_LENGTH + 1];
} controldesc_t;

typedef struct controlclass_s {
	uint    count;
	controldesc_t *desc;
} controlclass_t;
*/
/**
 * Each player has his own control state table.
 */
/*typedef struct controlstate_s {
	// The axes are updated whenever their values are needed,
	// i.e. during the call to P_BuildCommand.
	controlaxis_t *axes;

	// The toggles are modified via console commands.
	controltoggle_t *toggles;

	// The triggered impulses are stored into a ring buffer.
	uint    head, tail;
	impulse_t impulses[MAX_IMPULSES];
} controlstate_t;
*/

typedef struct doubleclick_s {
    boolean triggered;                      // True if double-click has been detected.
    uint    lastClickTime;
    boolean previousState;                  // State on the previous check.
} doubleclick_t;

typedef struct controlcounter_s {
    int     control;
    short   impulseCounts[DDMAXPLAYERS];
    doubleclick_t doubleClicks[DDMAXPLAYERS];    
} controlcounter_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

D_CMD(ListPlayerControls);
D_CMD(ClearControlAccumulation);
D_CMD(Impulse);

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------
/*
// Control class names - [singular, plural].
const char *ctlClassNames[NUM_CONTROL_CLASSES][NUM_CONTROL_CLASSES] = {
    {{"Axis"}, {"Axes"}},
    {{"Toggle"}, {"Toggles"}},
    {{"Impulse"}, {"Impulses"}}
};*/

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static playercontrol_t* playerControls;
static controlcounter_t** controlCounts;
static int playerControlCount;

// CODE --------------------------------------------------------------------

static playercontrol_t* P_AllocPlayerControl(void)
{
    playerControls = M_Realloc(playerControls, sizeof(playercontrol_t) *
                               ++playerControlCount);
    controlCounts = M_Realloc(controlCounts, sizeof(controlcounter_t*) *
                              playerControlCount);
    memset(&playerControls[playerControlCount - 1], 0, sizeof(playercontrol_t));
    controlCounts[playerControlCount - 1] = NULL;
    return &playerControls[playerControlCount - 1];
}

/**
 * Register the console commands and cvars of the player controls subsystem.
 */
void P_ControlRegister(void)
{
    C_CMD("listcontrols",   "",     ListPlayerControls);
    C_CMD("impulse",        NULL,   Impulse);
    C_CMD("resetctlaccum",  "",     ClearControlAccumulation);
}

/**
 * This function is exported, so that plugins can register their controls.
 */
void P_NewPlayerControl(int id, controltype_t type, const char *name, const char* bindClass)
{
    playercontrol_t *pc = P_AllocPlayerControl();
    pc->id = id;
    pc->type = type;
    pc->name = strdup(name);
    pc->bindClassName = strdup(bindClass);

    if(type == CTLT_IMPULSE)
    {
        // Also allocate the impulse counter.
        controlCounts[pc - playerControls] = M_Calloc(sizeof(controlcounter_t));
    }
}

playercontrol_t* P_PlayerControlById(int id)
{
    int     i;

    for(i = 0; i < playerControlCount; ++i)
    {
        if(playerControls[i].id == id)
            return playerControls + i;
    }
    return NULL;
}

playercontrol_t* P_PlayerControlByName(const char* name)
{
    int     i;

    for(i = 0; i < playerControlCount; ++i)
    {
        if(!strcasecmp(playerControls[i].name, name))
            return playerControls + i;
    }
    return NULL;
}

void P_ControlShutdown(void)
{
    int     i;

    for(i = 0; i < playerControlCount; ++i)
    {
        free(playerControls[i].name);
        free(playerControls[i].bindClassName);
        M_Free(controlCounts[i]);
    }
    playerControlCount = 0;
    M_Free(playerControls);
    playerControls = 0;
    M_Free(controlCounts);
    controlCounts = 0;
}

void P_GetControlState(int playerNum, int control, float* pos, float* relativeOffset)
{
    float tmp;
    struct bclass_s* bc = 0;
    struct dbinding_s* binds = 0;

#if _DEBUG
    // Check that this is really a numeric control.
    {
        playercontrol_t* pc = P_PlayerControlById(control);
        assert(pc);
        assert(pc->type == CTLT_NUMERIC);
    }
#endif

    // Ignore NULLs.
    if(!pos) pos = &tmp;
    if(!relativeOffset) relativeOffset = &tmp;

    binds = B_GetControlDeviceBindings(P_ConsoleToLocal(playerNum), control, &bc);
    B_EvaluateDeviceBindingList(binds, pos, relativeOffset, bc);
}

/**
 * @return  Number of times the impulse has been triggered since the last call.
 */
int P_GetImpulseControlState(int playerNum, int control)
{
    playercontrol_t* pc = P_PlayerControlById(control);
    short *counter;
    int count = 0;

    assert(pc != 0);

#if _DEBUG
    // Check that this is really an impulse control.
    assert(pc->type == CTLT_IMPULSE);
#endif
    if(!controlCounts[pc - playerControls])
        return 0;

    counter = &controlCounts[pc - playerControls]->impulseCounts[playerNum];
    count = *counter;
    *counter = 0;
    return count;
}

int P_GetControlDoubleClick(int playerNum, int control)
{
    playercontrol_t* pc = P_PlayerControlById(control);
    doubleclick_t *doubleClick = 0;
    int triggered = false;
    
    if(!pc || playerNum < 0 || playerNum >= DDMAXPLAYERS)
        return 0;
    
    if(controlCounts[pc - playerControls])
    {
        doubleClick = &controlCounts[pc - playerControls]->doubleClicks[playerNum];
        if(doubleClick->triggered)
        {
            triggered = true;
            doubleClick->triggered = false;    
        }
    }
    return triggered;
}

void P_Impulse(int playerNum, int control)
{
    playercontrol_t* pc = P_PlayerControlById(control);

    assert(pc);

    // Check that this is really an impulse control.
    if(pc->type != CTLT_IMPULSE)
    {
        Con_Message("P_Impulse: Control '%s' is not an impulse control.\n", pc->name);
        return;
    }

    if(playerNum < 0 || playerNum >= DDMAXPLAYERS)
        return;

    controlCounts[pc - playerControls]->impulseCounts[playerNum]++;
}

void P_ImpulseByName(int playerNum, const char* control)
{
    playercontrol_t* pc = P_PlayerControlByName(control);
    if(pc)
    {
        P_Impulse(playerNum, pc->id);
    }
}

void P_ControlTicker(timespan_t time)
{
    // Check for double-clicks.
    
}

D_CMD(ClearControlAccumulation)
{
    int     i, p;
    playercontrol_t* pc;

    for(i = 0; i < playerControlCount; ++i)
    {
        pc = &playerControls[i];
        for(p = 0; p < DDMAXPLAYERS; ++p)
        {
            if(pc->type == CTLT_NUMERIC)
                P_GetControlState(p, pc->id, NULL, NULL);
            else if(pc->type == CTLT_IMPULSE)
                P_GetImpulseControlState(p, pc->id);
            // Also clear the double click state.
            P_GetControlDoubleClick(p, pc->id);
        }
    }
    return true;
}

/**
 * Prints a list of the registered control descriptors.
 */
D_CMD(ListPlayerControls)
{
    /*
    uint        i, j;
	char        buf[MAX_DESCRIPTOR_LENGTH+1];

    Con_Message("Player Controls:\n");
    for(i = 0; i < NUM_CONTROL_CLASSES; ++i)
    {
        controlclass_t *cClass = &ctlClass[i];

        if(cClass->count > 0)
        {
            Con_Message("%i %s:\n", cClass->count,
                        ctlClassNames[i][cClass->count > 1]);
            for(j = 0; j < cClass->count; ++j)
            {
		        strncpy(buf, cClass->desc[j].name, sizeof(buf) - 1);
		        strlwr(buf);
                buf[strlen(cClass->desc[j].name)] = 0;
                Con_Message("  %s\n", buf);
            }
        }
    }*/
    return true;
}

D_CMD(Impulse)
{
    int playerNum = consolePlayer;

    if(argc < 2 || argc > 3)
    {
        Con_Printf("Usage:\n  %s (impulse-name)\n  %s (impulse-name) (player-number)\n",
                   argv[0], argv[0]);
        return true;
    }
    if(argc == 3)
    {
        playerNum = strtoul(argv[2], NULL, 10);
    }
    P_ImpulseByName(playerNum, argv[1]);
    return true;
}
