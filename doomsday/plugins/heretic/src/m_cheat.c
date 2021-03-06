/** @file m_cheat.c Cheat code sequences
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 1999 Activision
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

#include <stdlib.h>
#include <errno.h>

#include "jheretic.h"

#include "d_net.h"
#include "p_player.h"
#include "am_map.h"
#include "hu_msg.h"
#include "dmu_lib.h"
#include "p_user.h"
#include "p_inventory.h"
#include "g_eventsequence.h"

typedef eventsequencehandler_t cheatfunc_t;

/// Helper macro for forming cheat callback function names.
#define CHEAT(x) G_Cheat##x

/// Helper macro for declaring cheat callback functions.
#define CHEAT_FUNC(x) int G_Cheat##x(int player, const EventSequenceArg* args, int numArgs)

/// Helper macro for registering new cheat event sequence handlers.
#define ADDCHEAT(name, callback) G_AddEventSequence((name), CHEAT(callback))

/// Helper macro for registering new cheat event sequence command handlers.
#define ADDCHEATCMD(name, cmdTemplate) G_AddEventSequenceCommand((name), cmdTemplate)

CHEAT_FUNC(InvItem);
CHEAT_FUNC(InvItem2);
CHEAT_FUNC(InvItem3);
CHEAT_FUNC(IDKFA);
CHEAT_FUNC(IDDQD);
CHEAT_FUNC(Reveal);

void G_RegisterCheats(void)
{
    ADDCHEATCMD("cockadoodledoo",   "chicken %p");
    ADDCHEATCMD("engage%1%2",       "warp %1%2");
    ADDCHEAT("gimme%1%2",           InvItem3); // Final stage.
    ADDCHEAT("gimme%1",             InvItem2); // 2nd stage (ask for count).
    ADDCHEAT("gimme",               InvItem);  // 1st stage (ask for type).
    ADDCHEAT("iddqd",               IDDQD);
    ADDCHEAT("idkfa",               IDKFA);
    ADDCHEATCMD("kitty",            "noclip %p");
    ADDCHEATCMD("massacre",         "kill");
    ADDCHEATCMD("noise",            "playsound dorcls"); // ignored, play sound
    ADDCHEATCMD("ponce",            "give h %p");
    ADDCHEATCMD("quicken",          "god %p");
    ADDCHEATCMD("rambo",            "give wpar2 %p");
    ADDCHEAT("ravmap",              Reveal);
    ADDCHEATCMD("shazam",           "give t %p");
    ADDCHEATCMD("skel",             "give k %p");
    ADDCHEATCMD("ticker",           "playsound dorcls"); // ignored, play sound
}

CHEAT_FUNC(InvItem)
{
    DENG_UNUSED(args);
    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    P_SetMessage(&players[player], LMF_NO_HIDE, TXT_CHEATINVITEMS1);
    S_LocalSound(SFX_DORCLS, NULL);

    return true;
}

CHEAT_FUNC(InvItem2)
{
    DENG_UNUSED(args);
    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    P_SetMessage(&players[player], LMF_NO_HIDE, TXT_CHEATINVITEMS2);
    S_LocalSound(SFX_DORCLS, NULL);

    return true;
}

CHEAT_FUNC(InvItem3)
{
    player_t *plr = &players[player];
    inventoryitemtype_t type;
    int count;

    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    if(gameSkill == SM_NIGHTMARE) return false;
    // Dead players can't cheat.
    if(plr->health <= 0) return false;

    type  = args[0] - 'a' + 1;
    count = args[1] - '0';
    if(type > IIT_NONE && type < NUM_INVENTORYITEM_TYPES && count > 0 && count < 10)
    {
        int i;
        if(gameMode == heretic_shareware && (type == IIT_SUPERHEALTH || type == IIT_TELEPORT))
        {
            P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATITEMSFAIL);
            return false;
        }

        for(i = 0; i < count; ++i)
        {
            P_InventoryGive(player, type, false);
        }
        P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATINVITEMS3);
    }
    else
    {
        // Bad input
        P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATITEMSFAIL);
    }

    S_LocalSound(SFX_DORCLS, NULL);
    return true;
}

CHEAT_FUNC(IDKFA)
{
    player_t *plr = &players[player];
    int i;

    DENG_UNUSED(args);
    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    if(gameSkill == SM_NIGHTMARE) return false;
    // Dead players can't cheat.
    if(plr->health <= 0) return false;
    if(plr->morphTics) return false;

    plr->update |= PSF_OWNED_WEAPONS;
    for(i = 0; i < NUM_WEAPON_TYPES; ++i)
    {
        plr->weapons[i].owned = false;
    }

    //plr->pendingWeapon = WT_FIRST;
    P_MaybeChangeWeapon(plr, WT_FIRST, AT_NOAMMO, true /*force*/);

    P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATIDKFA);
    S_LocalSound(SFX_DORCLS, NULL);

    return true;
}

CHEAT_FUNC(IDDQD)
{
    player_t *plr = &players[player];

    DENG_UNUSED(args);
    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    if(gameSkill == SM_NIGHTMARE) return false;
    // Dead players can't cheat.
    if(plr->health <= 0) return false;

    P_DamageMobj(plr->plr->mo, NULL, plr->plr->mo, 10000, false);

    P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATIDDQD);
    S_LocalSound(SFX_DORCLS, NULL);

    return true;
}

CHEAT_FUNC(Reveal)
{
    player_t *plr = &players[player];

    DENG_UNUSED(args);
    DENG_ASSERT(player >= 0 && player < MAXPLAYERS);

    if(IS_NETGAME && deathmatch) return false;
    // Dead players can't cheat.
    if(plr->health <= 0) return false;

    if(ST_AutomapIsActive(player))
    {
        ST_CycleAutomapCheatLevel(player);
    }
    return true;
}

/**
 * The multipurpose cheat ccmd.
 */
D_CMD(Cheat)
{
    // Give each of the characters in argument two to the SB event handler.
    int i, len = (int) strlen(argv[1]);
    for(i = 0; i < len; ++i)
    {
        event_t ev;
        ev.type  = EV_KEY;
        ev.state = EVS_DOWN;
        ev.data1 = argv[1][i];
        ev.data2 = ev.data3 = 0;
        G_EventSequenceResponder(&ev);
    }
    return true;
}

D_CMD(CheatGod)
{
    if(G_GameState() == GS_MAP)
    {
        if(IS_CLIENT)
        {
            NetCl_CheatRequest("god");
        }
        else if((IS_NETGAME && !netSvAllowCheats) || gameSkill == SM_NIGHTMARE)
        {
            return false;
        }
        else
        {
            int player = CONSOLEPLAYER;
            player_t *plr;

            if(argc == 2)
            {
                player = atoi(argv[1]);
                if(player < 0 || player >= MAXPLAYERS) return false;
            }

            plr = &players[player];
            if(!plr->plr->inGame) return false;

            // Dead players can't cheat.
            if(plr->health <= 0) return false;

            plr->cheats ^= CF_GODMODE;
            plr->update |= PSF_STATE;

            P_SetMessage(plr, LMF_NO_HIDE, ((P_GetPlayerCheats(plr) & CF_GODMODE) ? TXT_CHEATGODON : TXT_CHEATGODOFF));
            S_LocalSound(SFX_DORCLS, NULL);
        }
    }
    return true;
}

D_CMD(CheatNoClip)
{
    if(G_GameState() == GS_MAP)
    {
        if(IS_CLIENT)
        {
            NetCl_CheatRequest("noclip");
        }
        else if((IS_NETGAME && !netSvAllowCheats) || gameSkill == SM_NIGHTMARE)
        {
            return false;
        }
        else
        {
            int player = CONSOLEPLAYER;
            player_t *plr;

            if(argc == 2)
            {
                player = atoi(argv[1]);
                if(player < 0 || player >= MAXPLAYERS) return false;
            }

            plr = &players[player];
            if(!plr->plr->inGame) return false;

            // Dead players can't cheat.
            if(plr->health <= 0) return false;

            plr->cheats ^= CF_NOCLIP;
            plr->update |= PSF_STATE;

            P_SetMessage(plr, LMF_NO_HIDE, ((P_GetPlayerCheats(plr) & CF_NOCLIP) ? TXT_CHEATNOCLIPON : TXT_CHEATNOCLIPOFF));
            S_LocalSound(SFX_DORCLS, NULL);
        }
    }
    return true;
}

static int suicideResponse(msgresponse_t response, int userValue, void *userPointer)
{
    if(response == MSG_YES)
    {
        if(IS_NETGAME && IS_CLIENT)
        {
            NetCl_CheatRequest("suicide");
        }
        else
        {
            player_t *plr = &players[CONSOLEPLAYER];
            P_DamageMobj(plr->plr->mo, NULL, NULL, 10000, false);
        }
    }
    return true;
}

D_CMD(CheatSuicide)
{
    if(G_GameState() == GS_MAP)
    {
        player_t *plr;

        if(IS_NETGAME && !netSvAllowCheats) return false;

        if(argc == 2)
        {
            int i = atoi(argv[1]);
            if(i < 0 || i >= MAXPLAYERS) return false;
            plr = &players[i];
        }
        else
        {
            plr = &players[CONSOLEPLAYER];
        }

        if(!plr->plr->inGame) return false;
        if(plr->playerState == PST_DEAD) return false;

        if(!IS_NETGAME || IS_CLIENT)
        {
            Hu_MsgStart(MSG_YESNO, SUICIDEASK, suicideResponse, 0, NULL);
            return true;
        }

        P_DamageMobj(plr->plr->mo, NULL, NULL, 10000, false);
        return true;
    }
    else
    {
        Hu_MsgStart(MSG_ANYKEY, SUICIDEOUTMAP, NULL, 0, NULL);
    }

    return true;
}

D_CMD(CheatReveal)
{
    int option, i;

    // Server operator can always reveal.
    if(IS_NETGAME && !IS_NETWORK_SERVER)
        return false;

    option = atoi(argv[1]);
    if(option < 0 || option > 3) return false;

    for(i = 0; i < MAXPLAYERS; ++i)
    {
        ST_SetAutomapCheatLevel(i, 0);
        ST_RevealAutomap(i, false);
        if(option == 1)
        {
            ST_RevealAutomap(i, true);
        }
        else if(option != 0)
        {
            ST_SetAutomapCheatLevel(i, option -1);
        }
    }

    return true;
}

D_CMD(CheatGive)
{
    char buf[100];
    int player = CONSOLEPLAYER;
    player_t *plr;
    size_t i, stuffLen;

    if(G_GameState() != GS_MAP)
    {
        Con_Printf("Can only \"give\" when in a game!\n");
        return true;
    }

    if(argc != 2 && argc != 3)
    {
        Con_Printf("Usage:\n  give (stuff)\n");
        Con_Printf("  give (stuff) (plr)\n");
        Con_Printf("Stuff consists of one or more of (type:id). "
                   "If no id; give all of type:\n");
        Con_Printf(" a - ammo\n");
        Con_Printf(" i - items\n");
        Con_Printf(" h - health\n");
        Con_Printf(" k - keys\n");
        Con_Printf(" p - backpack full of ammo\n");
        Con_Printf(" r - armor\n");
        Con_Printf(" t - tome of power\n");
        Con_Printf(" w - weapons\n");
        Con_Printf("Example: 'give ikw' gives items, keys and weapons.\n");
        Con_Printf("Example: 'give w2k1' gives weapon two and key one.\n");
        return true;
    }

    if(argc == 3)
    {
        player = atoi(argv[2]);
        if(player < 0 || player >= MAXPLAYERS) return false;
    }

    if(IS_CLIENT)
    {
        if(argc < 2) return false;

        sprintf(buf, "give %s", argv[1]);
        NetCl_CheatRequest(buf);
        return true;
    }

    if((IS_NETGAME && !netSvAllowCheats) || gameSkill == SM_NIGHTMARE)
        return false;

    plr = &players[player];

    // Can't give to a player who's not in the game.
    if(!plr->plr->inGame) return false;

    // Can't give to a dead player.
    if(plr->health <= 0) return false;

    strcpy(buf, argv[1]); // Stuff is the 2nd arg.
    strlwr(buf);
    stuffLen = strlen(buf);
    for(i = 0; buf[i]; ++i)
    {
        switch(buf[i])
        {
        case 'a':
            if(i < stuffLen)
            {
                char *end;
                long idx;
                errno = 0;
                idx = strtol(&buf[i+1], &end, 0);
                if(end != &buf[i+1] && errno != ERANGE)
                {
                    i += end - &buf[i+1];
                    if(idx < AT_FIRST || idx >= NUM_AMMO_TYPES)
                    {
                        Con_Printf("Unknown ammo #%d (valid range %d-%d).\n",
                                   (int)idx, AT_FIRST, NUM_AMMO_TYPES-1);
                        break;
                    }

                    // Give one specific ammo type.
                    P_GiveAmmo(plr, (ammotype_t) idx, -1 /*fully replenish*/);
                    break;
                }
            }

            // Give all ammo.
            P_GiveAmmo(plr, NUM_AMMO_TYPES, -1 /*fully replenish*/);
            break;

        case 'i': // Inventory items.
            if(i < stuffLen)
            {
                char* end;
                long idx;
                errno = 0;
                idx = strtol(&buf[i+1], &end, 0);
                if(end != &buf[i+1] && errno != ERANGE)
                {
                    i += end - &buf[i+1];
                    if(idx < IIT_FIRST || idx >= NUM_INVENTORYITEM_TYPES)
                    {
                        Con_Printf("Unknown item #%d (valid range %d-%d).\n",
                                   (int)idx, IIT_FIRST, NUM_INVENTORYITEM_TYPES-1);
                        break;
                    }

                    // Give one specific item type.
                    if(!(gameMode == heretic_shareware &&
                         (idx == IIT_SUPERHEALTH || idx == IIT_TELEPORT)))
                    {
                        int j;
                        for(j = 0; j < MAXINVITEMCOUNT; ++j)
                            P_InventoryGive(player, idx, false);
                    }
                    break;
                }
            }

            // Give all inventory items.
            { inventoryitemtype_t type;
            for(type = IIT_FIRST; type < NUM_INVENTORYITEM_TYPES; ++type)
            {
                if(gameMode == heretic_shareware &&
                   (type == IIT_SUPERHEALTH || type == IIT_TELEPORT))
                    continue;

                { int i;
                for(i = 0; i < MAXINVITEMCOUNT; ++i)
                    P_InventoryGive(player, type, false);
                }
            }}
            break;

        case 'h':
            P_GiveHealth(plr, -1 /*maximum amount*/);
            P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATHEALTH);
            S_LocalSound(SFX_DORCLS, NULL);
            break;

        case 'k':
            if(i < stuffLen)
            {
                char* end;
                long idx;
                errno = 0;
                idx = strtol(&buf[i+1], &end, 0);
                if(end != &buf[i+1] && errno != ERANGE)
                {
                    i += end - &buf[i+1];
                    if(idx < KT_FIRST || idx >= NUM_KEY_TYPES)
                    {
                        Con_Printf("Unknown key #%d (valid range %d-%d).\n",
                                   (int)idx, KT_FIRST, NUM_KEY_TYPES-1);
                        break;
                    }

                    // Give one specific key.
                    P_GiveKey(plr, (keytype_t)idx);
                    break;
                }
            }

            // Give all keys.
            P_GiveKey(plr, NUM_KEY_TYPES /*all types*/);
            P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATKEYS);
            S_LocalSound(SFX_DORCLS, NULL);
            break;

        case 'p':
            P_GiveBackpack(plr);
            break;

        case 'r': {
            int armorType = 2;

            if(i < stuffLen)
            {
                char *end;
                long idx;
                errno = 0;
                idx = strtol(&buf[i+1], &end, 0);
                if(end != &buf[i+1] && errno != ERANGE)
                {
                    i += end - &buf[i+1];
                    if(idx < 0 || idx >= 3)
                    {
                        Con_Printf("Unknown armor type #%d (valid range %d-%d).\n",
                                   (int)idx, 0, 3-1);
                        break;
                    }

                    armorType = idx;
                }
            }

            P_GiveArmor(plr, armorType, armorType * 100);
            break; }

        case 't':
            if(plr->powers[PT_WEAPONLEVEL2])
            {
                P_TakePower(plr, PT_WEAPONLEVEL2);
                P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATPOWEROFF);
            }
            else
            {
                P_InventoryGive(player, IIT_TOMBOFPOWER, true /*silent*/);
                P_InventoryUse(player, IIT_TOMBOFPOWER, true /*silent*/);
                P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATPOWERON);
            }
            S_LocalSound(SFX_DORCLS, NULL);
            break;

        case 'w':
            if(i < stuffLen)
            {
                char* end;
                long idx;
                errno = 0;
                idx = strtol(&buf[i+1], &end, 0);
                if(end != &buf[i+1] && errno != ERANGE)
                {
                    i += end - &buf[i+1];
                    if(idx < WT_FIRST || idx >= NUM_WEAPON_TYPES)
                    {
                        Con_Printf("Unknown weapon #%d (valid range %d-%d).\n",
                                   (int)idx, WT_FIRST, NUM_WEAPON_TYPES-1);
                        break;
                    }

                    // Give one specific weapon.
                    P_GiveWeapon(plr, (weapontype_t) idx);
                    break;
                }
            }

            // Give all weapons.
            P_GiveWeapon(plr, NUM_WEAPON_TYPES /*all types*/);
            break;

        default: // Unrecognized.
            Con_Printf("What do you mean, '%c'?\n", buf[i]);
            break;
        }
    }

    // If the give expression matches that of a vanilla cheat code print the
    // associated confirmation message to the player's log.
    /// @todo fixme: Somewhat of kludge...
    if(!strcmp(buf, "wpar2"))
    {
        P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATWEAPONS);
        S_LocalSound(SFX_DORCLS, NULL);
    }

    return true;
}

D_CMD(CheatMassacre)
{
    if(G_GameState() == GS_MAP)
    {
        if(IS_CLIENT)
        {
            NetCl_CheatRequest("kill");
        }
        else if((IS_NETGAME && !netSvAllowCheats) || gameSkill == SM_NIGHTMARE)
        {
            return false;
        }
        else
        {
            P_Massacre();
            P_SetMessage(&players[CONSOLEPLAYER], LMF_NO_HIDE, TXT_CHEATMASSACRE);
            S_LocalSound(SFX_DORCLS, NULL);
        }
    }
    return true;
}

D_CMD(CheatWhere)
{
    player_t *plr = &players[CONSOLEPLAYER];
    char textBuffer[256];
    BspLeaf *sub;
    AutoStr *path, *mapPath;
    Uri *uri, *mapUri;

    if(!plr->plr->mo || !userGame) return true;

    mapUri = G_ComposeMapUri(gameEpisode, gameMap);
    mapPath = Uri_ToString(mapUri);
    sprintf(textBuffer, "MAP [%s]  X:%g  Y:%g  Z:%g",
            Str_Text(mapPath), plr->plr->mo->origin[VX], plr->plr->mo->origin[VY],
            plr->plr->mo->origin[VZ]);
    P_SetMessage(plr, LMF_NO_HIDE, textBuffer);
    Uri_Delete(mapUri);

    // Also print some information to the console.
    Con_Message("%s", textBuffer);
    sub = plr->plr->mo->bspLeaf;
    Con_Message("BspLeaf %i:", P_ToIndex(sub));

    uri = Materials_ComposeUri(P_GetIntp(sub, DMU_FLOOR_MATERIAL));
    path = Uri_ToString(uri);
    Con_Message("  FloorZ:%g Material:%s", P_GetDoublep(sub, DMU_FLOOR_HEIGHT), Str_Text(path));
    Uri_Delete(uri);

    uri = Materials_ComposeUri(P_GetIntp(sub, DMU_CEILING_MATERIAL));
    path = Uri_ToString(uri);
    Con_Message("  CeilingZ:%g Material:%s", P_GetDoublep(sub, DMU_CEILING_HEIGHT), Str_Text(path));
    Uri_Delete(uri);

    Con_Message("Player height:%g Player radius:%g",
                plr->plr->mo->height, plr->plr->mo->radius);

    return true;
}

/**
 * Exit the current map and go to the intermission.
 */
D_CMD(CheatLeaveMap)
{
    // Only the server operator can end the map this way.
    if(IS_NETGAME && !IS_NETWORK_SERVER)
        return false;

    if(G_GameState() != GS_MAP)
    {
        S_LocalSound(SFX_CHAT, NULL);
        Con_Printf("Can only exit a map when in a game!\n");
        return true;
    }

    G_LeaveMap(G_GetNextMap(gameEpisode, gameMap, false), 0, false);
    return true;
}

D_CMD(CheatMorph)
{
    if(G_GameState() == GS_MAP)
    {
        if(IS_CLIENT)
        {
            NetCl_CheatRequest("chicken");
        }
        else if((IS_NETGAME && !netSvAllowCheats) || gameSkill == SM_NIGHTMARE)
        {
            return false;
        }
        else
        {
            int player = CONSOLEPLAYER;
            player_t *plr;

            if(argc == 2)
            {
                player = atoi(argv[1]);
                if(player < 0 || player >= MAXPLAYERS) return false;
            }

            plr = &players[player];
            if(!plr->plr->inGame) return false;

            // Dead players can't cheat.
            if(plr->health <= 0) return false;

            if(plr->morphTics)
            {
                if(P_UndoPlayerMorph(plr))
                {
                    P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATCHICKENOFF);
                }
            }
            else if(P_MorphPlayer(plr))
            {
                P_SetMessage(plr, LMF_NO_HIDE, TXT_CHEATCHICKENON);
            }
            S_LocalSound(SFX_DORCLS, NULL);
        }
    }
    return true;
}
