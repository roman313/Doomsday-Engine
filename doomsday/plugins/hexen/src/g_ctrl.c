/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2006-2013 Daniel Swanson <danij@dengine.net>
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
 * g_ctrl.c: Control bindings - Hexen specific
 */

// HEADER FILES ------------------------------------------------------------

#include "jhexen.h"

#include "g_controls.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

/**
 * Register all the various player controls with Doomsday.
 */
void G_RegisterPlayerControls(void)
{
    /*
    typedef struct {
        char   *command;                // The command to execute.
    } control_t;

    control_t axisCts[] = {
        {"WALK"},
        {"SIDESTEP"},
        {"turn"},
        {"ZFLY"},
        {"look"},
        {"MAPPANX"},
        {"MAPPANY"},
        {""}  // terminate
    };

    control_t toggleCts[] = {
        {"ATTACK"},
        {"USE"},
        {"strafe"},
        {"SPEED"},
        {"JUMP"},
        {"mlook"},
        {"jlook"},
        {"mzoomin"},
        {"mzoomout"},
        {""}  // terminate
    };

    control_t impulseCts[] = {
        {"falldown"},
        {"lookcntr"},
        {"usearti"},
        {"panic"},
        {"torch"},
        {"health"},
        {"mystic"},
        {"krater"},
        {"spdboots"},
        {"blast"},
        {"teleport"},
        {"teleothr"},
        {"poison"},
        {"cantdie"},
        {"servant"},
        {"egg"},
        {"weapon1"},
        {"weapon2"},
        {"weapon3"},
        {"weapon4"},
        {"nextwpn"},
        {"prevwpn"},
        {"demostop"},
        {""}  // terminate
    };
    uint        i;

    // Axis controls.
    for(i = 0; axisCts[i].command[0]; ++i)
    {
        P_RegisterPlayerControl(CC_AXIS, axisCts[i].command);
    }

    // Toggle controls.
    for(i = 0; toggleCts[i].command[0]; ++i)
    {
        P_RegisterPlayerControl(CC_TOGGLE, toggleCts[i].command);
    }

    // Impulse controls.
    for(i = 0; impulseCts[i].command[0]; ++i)
    {
        P_RegisterPlayerControl(CC_IMPULSE, impulseCts[i].command);
    }*/
}
