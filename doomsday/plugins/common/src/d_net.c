/** @file d_net.c Common code related to net games.
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2006-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright © 2007 Jamie Jones <jamie_jones_au@yahoo.com.au>
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
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small>
 */

#include "common.h"
#include "g_common.h"
#include "d_net.h"
#include "p_player.h"
#include "hu_menu.h"
#include "p_start.h"
#include "fi_lib.h"
#include "doomsday.h"

D_CMD(SetColor);
D_CMD(SetMap);
#if __JHEXEN__
D_CMD(SetClass);
#endif
D_CMD(LocalMessage);

static void D_NetMessageEx(int player, const char* msg, boolean playSound);

extern int netSvAllowSendMsg;

float netJumpPower = 9;

static Writer *netWriter;
static Reader *netReader;

static void notifyAllowCheatsChange(void)
{
    if(IS_NETGAME && IS_NETWORK_SERVER && G_GameState() != GS_STARTUP)
    {
        AutoStr *msg = Str_Appendf(AutoStr_NewStd(), "--- CHEATS NOW %s ON THIS SERVER ---",
                                                     netSvAllowCheats? "ENABLED" : "DISABLED");
        NetSv_SendMessage(DDSP_ALL_PLAYERS, Str_Text(msg));
    }
}

void D_NetConsoleRegistration(void)
{
    C_VAR_CHARPTR("mapcycle",                       &mapCycle,          CVF_HIDE | CVF_NO_ARCHIVE, 0, 0);
    C_VAR_CHARPTR("server-game-mapcycle",           &mapCycle,          0, 0, 0);
    C_VAR_BYTE   ("server-game-mapcycle-noexit",    &mapCycleNoExit,    0, 0, 1);
    C_VAR_INT2   ("server-game-cheat",              &netSvAllowCheats,  0, 0, 1, notifyAllowCheatsChange);

    C_CMD        ("setcolor",   "i",    SetColor);
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    C_CMD        ("setmap",     "ii",   SetMap);
#else
    C_CMD        ("setmap",     "i",    SetMap);
#endif
#if __JHEXEN__
    C_CMD_FLAGS  ("setclass",   "i",    SetClass, CMDF_NO_DEDICATED);
#endif
    C_CMD        ("startcycle", "",     MapCycle);
    C_CMD        ("endcycle",   "",     MapCycle);
    C_CMD        ("message",    "s",    LocalMessage);
}

Writer* D_NetWrite(void)
{
    if(netWriter)
    {
        Writer_Delete(netWriter);
    }
    netWriter = Writer_NewWithDynamicBuffer(0 /*unlimited*/);
    return netWriter;
}

Reader* D_NetRead(const byte* buffer, size_t len)
{
    // Get rid of the old reader.
    if(netReader)
    {
        Reader_Delete(netReader);
    }
    netReader = Reader_NewWithBuffer(buffer, len);
    return netReader;
}

void D_NetClearBuffer(void)
{
    if(netReader) Reader_Delete(netReader);
    if(netWriter) Writer_Delete(netWriter);

    netReader = 0;
    netWriter = 0;
}

void NetSv_ApplyGameRulesFromConfig(void)
{
    if(IS_CLIENT)
        return;

    deathmatch      = cfg.netDeathmatch;
    noMonstersParm  = cfg.netNoMonsters;
    cfg.jumpEnabled = cfg.netJumping;

#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
    respawnMonsters = cfg.netRespawn;
#endif

#if __JHEXEN__
    randomClassParm = cfg.netRandomClass;
#endif

    NetSv_UpdateGameConfigDescription();
}

/**
 * Called when the network server starts.
 *
 * Duties include:
 * Updating global state variables and initializing all players' settings
 */
int D_NetServerStarted(int before)
{
    uint netMap, netEpisode;

    if(before) return true;

    // We're the server, so...
    cfg.playerColor[0] = PLR_COLOR(0, cfg.netColor);

#if __JHEXEN__
    cfg.playerClass[0] = cfg.netClass;
#elif __JHERETIC__
    cfg.playerClass[0] = PCLASS_PLAYER;
#endif
    P_ResetPlayerRespawnClasses();

    // Set the game parameters.
    NetSv_ApplyGameRulesFromConfig();

    // Hexen has translated map numbers.
#if __JHEXEN__
    netMap = P_TranslateMap(cfg.netMap);
#else
    netMap = cfg.netMap;
#endif

#if __JDOOM64__
    netEpisode = 0;
#else
    netEpisode = cfg.netEpisode;
#endif

    G_NewGame(cfg.netSkill, netEpisode, netMap, 0/*default*/);

    /// @todo Necessary?
    G_SetGameAction(GA_NONE);

    return true;
}

/**
 * Called when a network server closes.
 *
 * Duties include:
 * Restoring global state variables
 */
int D_NetServerClose(int before)
{
    if(!before)
    {
        P_ResetPlayerRespawnClasses();

        // Restore normal game state.
        deathmatch = false;
        noMonstersParm = false;
#if __JHEXEN__
        randomClassParm = false;
#endif
        D_NetMessage(CONSOLEPLAYER, "NETGAME ENDS");

        D_NetClearBuffer();
    }
    return true;
}

int D_NetConnect(int before)
{
    // We do nothing before the actual connection is made.
    if(before) return true;

    // After connecting we tell the server a bit about ourselves.
    NetCl_SendPlayerInfo();

    // Close the menu, the game begins!!
    //  advancedemo = false;
    Hu_MenuCommand(MCMD_CLOSE);
    return true;
}

int D_NetDisconnect(int before)
{
    if(before)
        return true;

    // Restore normal game state.
    deathmatch = false;
    noMonstersParm = false;
#if __JHEXEN__
    randomClassParm = false;
#endif

    D_NetClearBuffer();

    // Start demo.
    G_StartTitle();
    return true;
}

/*
void* D_NetWriteCommands(int numCommands, void* data)
{
    // It's time to send ticcmds to the server.
    // 'plrNumber' contains the number of commands.
    return NetCl_WriteCommands(data, numCommands);
}
void* D_NetReadCommands(size_t pktLength, void* data)
{
    // Read ticcmds sent by a client.
    // 'plrNumber' is the length of the packet.
    return NetSv_ReadCommands(data, pktLength);
}
*/

long int D_NetPlayerEvent(int plrNumber, int peType, void *data)
{
    // If this isn't a netgame, we won't react.
    if(!IS_NETGAME)
        return true;

    if(peType == DDPE_ARRIVAL)
    {
        boolean showmsg = true;

        if(IS_SERVER)
        {
            NetSv_NewPlayerEnters(plrNumber);
        }
        else if(plrNumber == CONSOLEPLAYER)
        {
            // We have arrived, the game should be begun.
            Con_Message("PE: (client) arrived in netgame.");
            G_ChangeGameState(GS_WAITING);
            showmsg = false;
        }
        else
        {
            // Client responds to new player?
            Con_Message("PE: (client) player %i has arrived.", plrNumber);
            G_DoReborn(plrNumber);
            //players[plrNumber].playerstate = PST_REBORN;
        }
        if(showmsg)
        {
            // Print a notification.
            AutoStr *str = AutoStr_New();
            Str_Appendf(str, "%s joined the game", Net_GetPlayerName(plrNumber));
            D_NetMessage(CONSOLEPLAYER, Str_Text(str));
        }
    }
    else if(peType == DDPE_EXIT)
    {
        AutoStr *str = AutoStr_New();

        Con_Message("PE: player %i has left.", plrNumber);

        players[plrNumber].playerState = PST_GONE;

        // Print a notification.
        Str_Appendf(str, "%s left the game", Net_GetPlayerName(plrNumber));
        D_NetMessage(CONSOLEPLAYER, Str_Text(str));

        if(IS_SERVER)
            P_DealPlayerStarts(0);
    }
    // DDPE_CHAT_MESSAGE occurs when a PKT_CHAT is received.
    // Here we will only display the message.
    else if(peType == DDPE_CHAT_MESSAGE)
    {
        int oldecho = cfg.echoMsg;
        AutoStr *msg = AutoStr_New();

        if(plrNumber > 0)
        {
            Str_Appendf(msg, "%s: %s", Net_GetPlayerName(plrNumber), (char const *) data);
        }
        else
        {
            Str_Appendf(msg, "[sysop] %s", (char const *) data);
        }
        Str_Truncate(msg, NETBUFFER_MAXMESSAGE); // not overly long, please

        // The chat message is already echoed by the console.
        cfg.echoMsg = false;
        D_NetMessageEx(CONSOLEPLAYER, Str_Text(msg), (cfg.chatBeep? true : false));
        cfg.echoMsg = oldecho;
    }

    return true;
}

int D_NetWorldEvent(int type, int parm, void* data)
{
    int i;

    switch(type)
    {
    //
    // Server events:
    //
    case DDWE_HANDSHAKE:
        {
        boolean newPlayer = *((boolean*) data);

        // A new player is entering the game. We as a server should send him
        // the handshake packet(s) to update his world.
        // If 'data' is zero, this is a re-handshake that's used to
        // begin demos.
        Con_Message("D_NetWorldEvent: Sending a %shandshake to player %i.",
                    newPlayer ? "" : "(re)", parm);

        // Mark new player for update.
        players[parm].update |= PSF_REBORN;

        // First, the game state.
        NetSv_SendGameState(GSF_CHANGE_MAP | GSF_CAMERA_INIT | (newPlayer ? 0 : GSF_DEMO), parm);

        // Send info about all players to the new one.
        for(i = 0; i < MAXPLAYERS; ++i)
            if(players[i].plr->inGame && i != parm)
                NetSv_SendPlayerInfo(i, parm);

        // Send info about our jump power.
        NetSv_SendJumpPower(parm, cfg.jumpEnabled ? cfg.jumpPower : 0);
        NetSv_Paused(paused);
        break;
        }

    //
    // Client events:
    //
#if 0
    case DDWE_SECTOR_SOUND:
        // High word: sector number, low word: sound id.
        if(parm & 0xffff)
            S_SectorSound(P_ToPtr(DMU_SECTOR, parm >> 16), parm & 0xffff);
        else
            S_SectorStopSounds(P_ToPtr(DMU_SECTOR, parm >> 16));

        break;

    case DDWE_DEMO_END:
        // Demo playback has ended. Advance demo sequence.
        if(parm)                // Playback was aborted.
            G_DemoAborted();
        else                    // Playback ended normally.
            G_DemoEnds();

        // Restore normal game state.
        deathmatch = false;
        noMonstersParm = false;
#if __JDOOM__ || __JHERETIC__ || __JDOOM64__
        respawnMonsters = false;
#endif

#if __JHEXEN__
        randomClassParm = false;
#endif
        break;
#endif

    default:
        return false;
    }
    return true;
}

void D_HandlePacket(int fromplayer, int type, void *data, size_t length)
{
    Reader* reader = D_NetRead(data, length);

    //
    // Server events.
    //
    if(IS_SERVER)
    {
        switch (type)
        {
        case GPT_PLAYER_INFO:
            // A player has changed color or other settings.
            NetSv_ChangePlayerInfo(fromplayer, reader);
            break;

        case GPT_CHEAT_REQUEST:
            NetSv_DoCheat(fromplayer, reader);
            break;

        case GPT_FLOOR_HIT_REQUEST:
            NetSv_DoFloorHit(fromplayer, reader);
            break;

        case GPT_ACTION_REQUEST:
            NetSv_DoAction(fromplayer, reader);
            break;

        case GPT_DAMAGE_REQUEST:
            NetSv_DoDamage(fromplayer, reader);
            break;
        }
        return;
    }
    //
    // Client events.
    //
    switch(type)
    {
    case GPT_GAME_STATE:
#ifdef _DEBUG
        Con_Message("Received GTP_GAME_STATE");
#endif
        NetCl_UpdateGameState(reader);

        // Tell the engine we're ready to proceed. It'll start handling
        // the world updates after this variable is set.
        Set(DD_GAME_READY, true);
        break;

    case GPT_PLAYER_SPAWN_POSITION:
        NetCl_PlayerSpawnPosition(reader);
        break;

    case GPT_TOTAL_COUNTS:
        NetCl_UpdateTotalCounts(reader);
        break;

    case GPT_MOBJ_IMPULSE:
        NetCl_MobjImpulse(reader);
        break;

    case GPT_LOCAL_MOBJ_STATE:
        NetCl_LocalMobjState(reader);
        break;

    case GPT_MESSAGE:
#if __JHEXEN__
    case GPT_YELLOW_MESSAGE:
#endif
    {
        size_t len = 0;
        char *msg = 0;
#ifdef _DEBUG
        Con_Message("D_HandlePacket: GPT_MESSAGE");
#endif
        len = Reader_ReadUInt16(reader);
        msg = Z_Malloc(len + 1, PU_GAMESTATIC, 0);
        Reader_Read(reader, msg, len);
        msg[len] = 0;

#if __JHEXEN__
        if(type == GPT_YELLOW_MESSAGE)
        {
            P_SetYellowMessage(&players[CONSOLEPLAYER], 0, msg);
        }
        else
#endif
        {
            P_SetMessage(&players[CONSOLEPLAYER], 0, msg);
        }
        Z_Free(msg);
        break;
    }

    case GPT_MAYBE_CHANGE_WEAPON:
    {
        weapontype_t wt = (weapontype_t) Reader_ReadInt16(reader);
        ammotype_t at = (ammotype_t) Reader_ReadInt16(reader);
        boolean force = (Reader_ReadByte(reader) != 0);
        P_MaybeChangeWeapon(&players[CONSOLEPLAYER], wt, at, force);
        break;
    }

    case GPT_CONSOLEPLAYER_STATE:
        NetCl_UpdatePlayerState(reader, CONSOLEPLAYER);
        break;

    case GPT_CONSOLEPLAYER_STATE2:
        NetCl_UpdatePlayerState2(reader, CONSOLEPLAYER);
        break;

    case GPT_PLAYER_STATE:
        NetCl_UpdatePlayerState(reader, -1);
        break;

    case GPT_PLAYER_STATE2:
        NetCl_UpdatePlayerState2(reader, -1);
        break;

    case GPT_PSPRITE_STATE:
        NetCl_UpdatePSpriteState(reader);
        break;

    case GPT_INTERMISSION:
        NetCl_Intermission(reader);
        break;

    case GPT_FINALE_STATE:
        NetCl_UpdateFinaleState(reader);
        break;

    case GPT_PLAYER_INFO:
        NetCl_UpdatePlayerInfo(reader);
        break;

#if __JHERETIC__ || __JHEXEN__ || __JSTRIFE__
    case GPT_CLASS:
    {
        player_t* plr = &players[CONSOLEPLAYER];
        int newClass = Reader_ReadByte(reader);
        int oldClass = plr->class_;
        plr->class_ = newClass;
#ifdef _DEBUG
        Con_Message("D_HandlePacket: Player %i class set to %i.", CONSOLEPLAYER, plr->class_);
#endif
#if __JHERETIC__
        if(oldClass != newClass)
        {
            if(newClass == PCLASS_CHICKEN)
            {
#ifdef _DEBUG
                Con_Message("D_HandlePacket: Player %i activating morph..", CONSOLEPLAYER);
#endif
                P_ActivateMorphWeapon(plr);
            }
            else if(oldClass == PCLASS_CHICKEN)
            {
#ifdef _DEBUG
                Con_Message("NetCl_UpdatePlayerState: Player %i post-morph weapon %i.", CONSOLEPLAYER, plr->readyWeapon);
#endif
                // The morph has ended.
                P_PostMorphWeapon(plr, plr->readyWeapon);
            }
        }
#endif
        break;
    }
#endif

    case GPT_SAVE:
        NetCl_SaveGame(reader);
        break;

    case GPT_LOAD:
        NetCl_LoadGame(reader);
        break;

    case GPT_PAUSE:
        NetCl_Paused(reader);
        break;

    case GPT_JUMP_POWER:
        NetCl_UpdateJumpPower(reader);
        break;

    default:
        Con_Message("H_HandlePacket: Received unknown packet, type=%i.", type);
    }
}

/**
 * Plays a (local) chat sound.
 */
void D_ChatSound(void)
{
#if __JHEXEN__
    S_LocalSound(SFX_CHAT, NULL);
#elif __JSTRIFE__
    S_LocalSound(SFX_CHAT, NULL);
#elif __JHERETIC__
    S_LocalSound(SFX_CHAT, NULL);
#else
# if __JDOOM__
    if(gameModeBits & GM_ANY_DOOM2)
        S_LocalSound(SFX_RADIO, NULL);
    else
# endif
        S_LocalSound(SFX_TINK, NULL);
#endif
}

/**
 * Show a message on screen, optionally accompanied by Chat sound effect.
 *
 * @param player        Player number to send the message to.
 * @param playSound     @c true = play the chat sound.
 */
static void D_NetMessageEx(int player, const char* msg, boolean playSound)
{
    player_t *plr;

    if(player < 0 || player > MAXPLAYERS)
        return;
    plr = &players[player];

    if(!plr->plr->inGame)
        return;

    // This is intended to be a local message.
    // Let's make sure P_SetMessage doesn't forward it anywhere.
    netSvAllowSendMsg = false;
    P_SetMessage(plr, 0, msg);

    if(playSound)
        D_ChatSound();

    netSvAllowSendMsg = true;
}

/**
 * Show message on screen and play chat sound.
 *
 * @param msg           Ptr to the message to print.
 */
void D_NetMessage(int player, const char* msg)
{
    D_NetMessageEx(player, msg, true);
}

/**
 * Show message on screen.
 *
 * @param msg
 */
void D_NetMessageNoSound(int player, const char* msg)
{
    D_NetMessageEx(player, msg, false);
}

/**
 * Issues a damage request when a client is trying to damage another player's
 * mobj.
 *
 * @return                  @c true = no further processing of the damage
 *                          should be done else, process the damage as
 *                          normally.
 */
boolean D_NetDamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source,
                        int damage)
{
    int sourcePlrNum = -1;

    if(source && source->player)
        sourcePlrNum = source->player - players;

    if(source && !source->player)
    {
        // Not applicable: only damage from players.
        return false;
    }

    if(IS_SERVER && sourcePlrNum > 0)
    {
        /*
         * A client is trying to do damage. However, it is not guaranteed
         * that the server is 100% accurately aware of the gameplay situation
         * in which the damage is being inflicted (due to network latency),
         * so instead of applying the damage now we will wait for the client
         * to request it separately.
         */
        return false;
    }
    else if(IS_CLIENT)
    {
        if((sourcePlrNum < 0 || sourcePlrNum == CONSOLEPLAYER) &&
           target && target->player && target->player - players == CONSOLEPLAYER)
        {
            // Clients are allowed to damage themselves.
            NetCl_DamageRequest(ClPlayer_ClMobj(CONSOLEPLAYER), inflictor, source, damage);

            // No further processing of this damage is needed.
            return true;
        }
    }
    return false;
}

/**
 * Console command to change the players' colors.
 */
D_CMD(SetColor)
{
    cfg.netColor = atoi(argv[1]);
    if(IS_SERVER) // A local player?
    {
        int                 player = CONSOLEPLAYER;

        if(IS_DEDICATED)
            return false;

        // Server players, must be treated as a special case because this is
        // a local mobj we're dealing with. We'll change the color translation
        // bits directly.

        cfg.playerColor[player] = PLR_COLOR(player, cfg.netColor);
        players[player].colorMap = cfg.playerColor[player];

        if(players[player].plr->mo)
        {
            // Change the color of the mobj (translation flags).
            players[player].plr->mo->flags &= ~MF_TRANSLATION;
            players[player].plr->mo->flags |= (cfg.playerColor[player] << MF_TRANSSHIFT);
        }

        // Tell the clients about the change.
        NetSv_SendPlayerInfo(player, DDSP_ALL_PLAYERS);
    }
    else
    {
        // Tell the server about the change.
        NetCl_SendPlayerInfo();
    }

    return true;
}

/**
 * Console command to change the players' class.
 */
#if __JHEXEN__
D_CMD(SetClass)
{
    playerclass_t newClass = atoi(argv[1]);

    if(!(newClass < NUM_PLAYER_CLASSES))
        return false;

    if(!PCLASS_INFO(newClass)->userSelectable)
        return false;

    cfg.netClass = newClass; // Stored as a cvar.

    if(IS_CLIENT)
    {
        // Tell the server that we want to change our class.
        NetCl_SendPlayerInfo();
    }
    else
    {
        // On the server (or singleplayer) we can do an immediate change.
        P_PlayerChangeClass(&players[CONSOLEPLAYER], cfg.netClass);
    }

    return true;
}
#endif

/**
 * Console command to change the current map.
 */
D_CMD(SetMap)
{
    uint ep, map;

    // Only the server can change the map.
    if(!IS_SERVER)
        return false;

#if __JDOOM__ || __JHERETIC__
    if(argc != 3)
    {
        Con_Printf("Usage: %s (episode) (map)\n", argv[0]);
        return true;
    }
#else
    if(argc != 2)
    {
        Con_Printf("Usage: %s (map)\n", argv[0]);
        return true;
    }
#endif

    // Parse arguments.
#if __JDOOM__ || __JHERETIC__
    ep = atoi(argv[1]);
    if(ep != 0) ep -= 1;
#else
    ep = 0;
#endif

#if __JDOOM__ || __JHERETIC__
    map = atoi(argv[2]); if(map != 0) map -= 1;
#else
    map = atoi(argv[1]); if(map != 0) map -= 1;
#endif
#if __JHEXEN__
    map = P_TranslateMapIfExists(map);
    if(map == P_INVALID_LOGICAL_MAP)
    {
        Con_Message("Map not found.");
        return false;
    }
#endif

    // Update game mode.
    deathmatch      = cfg.netDeathmatch;
    noMonstersParm  = cfg.netNoMonsters;
#if __JDOOM__ || __JDOOM64__ || __JHERETIC__
    respawnMonsters = cfg.netRespawn;
#endif
#if __JHEXEN__
    randomClassParm = cfg.netRandomClass;
#endif
    cfg.jumpEnabled = cfg.netJumping;

    // Use the configured network skill level for the new map.
#if __JHEXEN__
    G_DeferredSetMap(cfg.netSkill, ep, map, 0/*default*/);
#else
    G_DeferredNewGame(cfg.netSkill, ep, map, 0/*default*/);
#endif
    return true;
}

/**
 * Post a local game message.
 */
D_CMD(LocalMessage)
{
    D_NetMessageNoSound(CONSOLEPLAYER, argv[1]);
    return true;
}
