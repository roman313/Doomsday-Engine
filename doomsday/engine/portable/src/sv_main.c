/**\file
 *\section License
 * License: GPL
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2003-2011 Jaakko Keränen <jaakko.keranen@iki.fi>
 *\author Copyright © 2006-2011 Daniel Swanson <danij@dengine.net>
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
 * sv_main.c: Network Server
 */

// HEADER FILES ------------------------------------------------------------

#include <math.h>

#include "de_base.h"
#include "de_console.h"
#include "de_system.h"
#include "de_network.h"
#include "de_play.h"
#include "de_misc.h"

#include "r_world.h"

// MACROS ------------------------------------------------------------------

// This is absolute maximum bandwidth rating. Frame size is practically
// unlimited with this score.
#define MAX_BANDWIDTH_RATING    100

// When the difference between clientside and serverside positions is this
// much, server will update its position to match the clientside position,
// which is assumed to be correct.
#define WARP_LIMIT              300

// Maximum length of a token in the textual representation of
// serverinfo.
#define TOKEN_LEN 128
#define VALID_LABEL_LEN 16

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

void    Sv_ClientCoords(int playerNum);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int     netRemoteUser = 0; // The client who is currently logged in.
char   *netPassword = ""; // Remote login password.

// This is the limit when accepting new clients.
int     svMaxPlayers = DDMAXPLAYERS;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

/**
 * Fills the provided struct with information about the local server.
 */
void Sv_GetInfo(serverinfo_t *info)
{
    int                 i;
    gamemap_t          *currentMap = P_GetCurrentMap();

    memset(info, 0, sizeof(*info));

    // Let's figure out what we want to tell about ourselves.
    info->version = DOOMSDAY_VERSION;
    strncpy(info->game, gx.GetVariable(DD_GAME_ID), sizeof(info->game) - 1);
    strncpy(info->gameMode, gx.GetVariable(DD_GAME_MODE), sizeof(info->gameMode) - 1);
    strncpy(info->gameConfig, gx.GetVariable(DD_GAME_CONFIG),
            sizeof(info->gameConfig) - 1);
    strncpy(info->name, serverName, sizeof(info->name) - 1);
    strncpy(info->description, serverInfo, sizeof(info->description) - 1);
    info->numPlayers = Sv_GetNumPlayers();

    // The server player is there, it's just hidden.
    info->maxPlayers = DDMAXPLAYERS - (isDedicated ? 1 : 0);

    // Don't go over the limit.
    if(info->maxPlayers > svMaxPlayers)
        info->maxPlayers = svMaxPlayers;

    info->canJoin = (isServer != 0 && Sv_GetNumPlayers() < svMaxPlayers);

    // Identifier of the current map.
    strncpy(info->map, P_GetMapID(currentMap), sizeof(info->map) - 1);

    // These are largely unused at the moment... Mainly intended for
    // the game's custom values.
    memcpy(info->data, serverData, sizeof(info->data));

    // Also include the port we're using.
    info->port = nptIPPort;

    // Let's compile a list of client names.
    for(i = 0; i < DDMAXPLAYERS; ++i)
        if(clients[i].connected)
        {
            M_LimitedStrCat(info->clientNames, clients[i].name, 15, ';',
                            sizeof(info->clientNames));
        }

    // Some WAD names.
    W_GetIWADFileName(info->iwad, sizeof(info->iwad) - 1);
    W_GetPWADFileNames(info->pwads, sizeof(info->pwads), ';');

    // This should be a CRC number that describes all the loaded data.
    info->wadNumber = W_CRCNumber();
}

/**
 * @return              The length of the string.
 */
size_t Sv_InfoToString(serverinfo_t *info, ddstring_t *msg)
{
    unsigned int i;

    Str_Appendf(msg, "port:%i\n", info->port);
    Str_Appendf(msg, "name:%s\n", info->name);
    Str_Appendf(msg, "info:%s\n", info->description);
    Str_Appendf(msg, "ver:%i\n", info->version);
    Str_Appendf(msg, "game:%s\n", info->game);
    Str_Appendf(msg, "mode:%s\n", info->gameMode);
    Str_Appendf(msg, "setup:%s\n", info->gameConfig);
    Str_Appendf(msg, "iwad:%s\n", info->iwad);
    Str_Appendf(msg, "wcrc:%i\n", info->wadNumber);
    Str_Appendf(msg, "pwads:%s\n", info->pwads);
    Str_Appendf(msg, "map:%s\n", info->map);
    Str_Appendf(msg, "nump:%i\n", info->numPlayers);
    Str_Appendf(msg, "maxp:%i\n", info->maxPlayers);
    Str_Appendf(msg, "open:%i\n", info->canJoin);
    Str_Appendf(msg, "plrn:%s\n", info->clientNames);
    for(i = 0; i < sizeof(info->data) / sizeof(info->data[0]); ++i)
    {
        Str_Appendf(msg, "data%i:%x\n", i, info->data[i]);
    }
    return Str_Length(msg);
}

/**
 * Extracts the label and value from a string.  'max' is the maximum
 * allowed length of a token, including terminating \0.
 */
static boolean Sv_Tokenize(const char *line, char *label, char *value,
                           int max)
{
    const char *src = line;
    const char *colon = strchr(src, ':');

    // The colon must exist near the beginning.
    if(!colon || colon - src >= VALID_LABEL_LEN)
        return false;

    // Copy the label.
    memset(label, 0, max);
    strncpy(label, src, MIN_OF(colon - src, max - 1));

    // Copy the value.
    memset(value, 0, max);
    strncpy(value, colon + 1,
            MIN_OF(strlen(line) - (colon - src + 1), (unsigned) max - 1));

    // Everything is OK.
    return true;
}

/**
 * Converts textual data to a serverinfo struct. Returns true if the
 * label/value pair is recognized.
 */
boolean Sv_StringToInfo(const char *valuePair, serverinfo_t *info)
{
    char        label[TOKEN_LEN], value[TOKEN_LEN];

    // Extract the label and value. The maximum length of a value is
    // TOKEN_LEN. Labels are returned in lower case.
    if(!Sv_Tokenize(valuePair, label, value, sizeof(value)))
    {
        // Badly formed lines are ignored.
        return false;
    }

    if(!strcmp(label, "at"))
    {
        strncpy(info->address, value, sizeof(info->address) - 1);
    }
    else if(!strcmp(label, "port"))
    {
        info->port = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "ver"))
    {
        info->version = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "map"))
    {
        strncpy(info->map, value, sizeof(info->map) - 1);
    }
    else if(!strcmp(label, "game"))
    {
        strncpy(info->game, value, sizeof(info->game) - 1);
    }
    else if(!strcmp(label, "name"))
    {
        strncpy(info->name, value, sizeof(info->name) - 1);
    }
    else if(!strcmp(label, "info"))
    {
        strncpy(info->description, value, sizeof(info->description) - 1);
    }
    else if(!strcmp(label, "nump"))
    {
        info->numPlayers = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "maxp"))
    {
        info->maxPlayers = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "open"))
    {
        info->canJoin = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "mode"))
    {
        strncpy(info->gameMode, value, sizeof(info->gameMode) - 1);
    }
    else if(!strcmp(label, "setup"))
    {
        strncpy(info->gameConfig, value, sizeof(info->gameConfig) - 1);
    }
    else if(!strcmp(label, "iwad"))
    {
        strncpy(info->iwad, value, sizeof(info->iwad) - 1);
    }
    else if(!strcmp(label, "wcrc"))
    {
        info->wadNumber = strtol(value, 0, 0);
    }
    else if(!strcmp(label, "pwads"))
    {
        strncpy(info->pwads, value, sizeof(info->pwads) - 1);
    }
    else if(!strcmp(label, "plrn"))
    {
        strncpy(info->clientNames, value, sizeof(info->clientNames) - 1);
    }
    else if(!strcmp(label, "data0"))
    {
        info->data[0] = strtol(value, 0, 16);
    }
    else if(!strcmp(label, "data1"))
    {
        info->data[1] = strtol(value, 0, 16);
    }
    else if(!strcmp(label, "data2"))
    {
        info->data[2] = strtol(value, 0, 16);
    }
    else
    {
        // Unknown labels are ignored.
        return false;
    }
    return true;
}

/**
 * @return              gametic - cmdtime.
 */
int Sv_Latency(byte cmdtime)
{
    return Net_TimeDelta(SECONDS_TO_TICKS(gameTime), cmdtime);
}

/**
 * For local players.
 */
/* $unifiedangles */
/*
void Sv_FixLocalAngles(boolean clearFixAnglesFlag)
{
    ddplayer_t *pl;
    int         i;

    for(i = 0; i < DDMAXPLAYERS; ++i)
    {
        pl = players + i;
        if(!pl->inGame || !(pl->flags & DDPF_LOCAL))
            continue;

        // This is not for clients.
        if(isDedicated && i == 0)
            continue;

        if(pl->flags & DDPF_FIXANGLES)
        {
            if(clearFixAnglesFlag)
            {
                pl->flags &= ~DDPF_FIXANGLES;
            }
            else
            {
                pl->clAngle = pl->mo->angle;
                pl->clLookDir = pl->lookDir;
            }
        }
    }
}
*/

void Sv_HandlePlayerInfoFromClient(client_t* sender)
{
    int console = Reader_ReadByte(msgReader); // ignored
    char oldName[PLAYERNAMELEN];
    size_t len;

    assert(netBuffer.player == (sender - clients));
#ifdef _DEBUG
    Con_Message("Sv_HandlePlayerInfoFromClient: from=%i, console=%i\n", netBuffer.player, console);
#endif
    console = netBuffer.player;

    strcpy(oldName, sender->name);

    len = Reader_ReadUInt16(msgReader);
    len = MIN_OF(PLAYERNAMELEN - 1, len); // there is a maximum size
    Reader_Read(msgReader, sender->name, len);
    sender->name[len] = 0;
    Con_FPrintf(CBLF_TRANSMIT | SV_CONSOLE_FLAGS, "%s renamed to %s.\n", oldName, sender->name);

    // Relay to others.
    Net_SendPlayerInfo(console, DDSP_ALL_PLAYERS);
}

/**
 * Handles a server-specific network message. Assumes that Msg_BeginRead()
 * has already been called to begin reading the message.
 */
void Sv_HandlePacket(void)
{
    ident_t             id;
    int                 i, mask, from = netBuffer.player;
    player_t           *plr = &ddPlayers[from];
    ddplayer_t         *ddpl = &plr->shared;
    client_t           *sender = &clients[from];
    int                 msgfrom;
    char               *msg;
    char                buf[17];
    size_t              len;

    /*
#ifdef _DEBUG
    Con_Message("Sv_HandlePacket: type=%i\n", netBuffer.msg.type);
    Con_Message("Sv_HandlePacket: length=%li\n", netBuffer.length);
#endif
    */

    switch(netBuffer.msg.type)
    {
    case PCL_HELLO:
    case PCL_HELLO2:
        // Get the ID of the client.
        id = Reader_ReadUInt32(msgReader);
        Con_Printf("Sv_HandlePacket: Hello from client %i (%08X).\n",
                   from, id);

        // Check for duplicate IDs.
        if(!ddpl->inGame && !sender->handshake)
        {
            for(i = 0; i < DDMAXPLAYERS; ++i)
            {
                if(clients[i].connected && clients[i].id == id)
                {
                    // Send a message to everybody.
                    Con_FPrintf(CBLF_TRANSMIT | SV_CONSOLE_FLAGS,
                                "New client connection refused: Duplicate ID "
                                "(%08x). From=%i, i=%i\n", id, from, i);
                    N_TerminateClient(from);
                    break;
                }
            }

            if(i < DDMAXPLAYERS)
                break; // Can't continue, refused!
        }

        // This is OK.
        sender->id = id;

        if(netBuffer.msg.type == PCL_HELLO2)
        {
            // Check the game mode (max 16 chars).
            Reader_Read(msgReader, buf, 16);
            if(strnicmp(buf, gx.GetVariable(DD_GAME_MODE), 16))
            {
                Con_Printf("  Bad Game ID: %-.16s (expected %s)\n", buf, (char*)gx.GetVariable(DD_GAME_MODE));
                N_TerminateClient(from);
                break;
            }
        }

        // The client requests a handshake.
        if(!ddpl->inGame && !sender->handshake)
        {
            // This'll be true until the client says it's ready.
            sender->handshake = true;

            // The player is now in the game.
            ddPlayers[from].shared.inGame = true;

            // Tell the game about this.
            gx.NetPlayerEvent(from, DDPE_ARRIVAL, 0);

            // Send the handshake packets.
            Sv_Handshake(from, true);

            // Note the time when the player entered.
            sender->enterTime = Sys_GetRealSeconds();
        }
        else if(ddpl->inGame)
        {
            // The player is already in the game but requests a new
            // handshake. Perhaps it's starting to record a demo.
            Sv_Handshake(from, false);
        }
        break;

    case PKT_OK:
        // The client says it's ready to receive frames.
        sender->ready = true;
#ifdef _DEBUG
Con_Printf("Sv_HandlePacket: OK (\"ready!\") from client %i "
           "(%08X).\n", from, sender->id);
#endif
        if(sender->handshake)
        {
            // The handshake is complete. The client has acknowledged it
            // and sends its regards.
            sender->handshake = false;
            // Send a clock sync message.
            Msg_Begin(PSV_SYNC);
            Writer_WriteFloat(msgWriter, gameTime);
            Msg_End();
            Net_SendBuffer(from, 0);
            // Send welcome string.
            Sv_SendText(from, SV_CONSOLE_FLAGS, SV_WELCOME_STRING "\n");
        }
        break;

    case PKT_CHAT:
        // The first byte contains the sender.
        msgfrom = Reader_ReadByte(msgReader);
        // Is the message for us?
        mask = Reader_ReadUInt32(msgReader);
        // Copy the message into a buffer.
        len = Reader_ReadUInt16(msgReader);
        msg = M_Malloc(len + 1);
        Reader_Read(msgReader, msg, len);
        msg[len] = 0;
        // Message for us? Show it locally.
        if(mask & 1)
        {
            Net_ShowChatMessage(msgfrom, msg);
            gx.NetPlayerEvent(msgfrom, DDPE_CHAT_MESSAGE, msg);
        }

        // Servers relay chat messages to all the recipients.
        Net_WriteChatMessage(msgfrom, mask, msg);
        for(i = 1; i < DDMAXPLAYERS; ++i)
            if(ddPlayers[i].shared.inGame && (mask & (1 << i)) && i != from)
            {
                Net_SendBuffer(i, 0);
            }
        M_Free(msg);
        break;

    case PKT_PLAYER_INFO:
        Sv_HandlePlayerInfoFromClient(sender);
        break;

    default:
        Con_Error("Sv_HandlePacket: Invalid value, netBuffer.msg.type = %i.",
                  (int) netBuffer.msg.type);
        break;
    }
}

/**
 * Handles a login packet. If the password is OK and no other client
 * is current logged in, a response is sent.
 */
void Sv_Login(void)
{
    char password[300];
    byte passLen = 0;

    if(netRemoteUser)
    {
        Sv_SendText(netBuffer.player, SV_CONSOLE_FLAGS,
                    "Sv_Login: A client is already logged in.\n");
        return;
    }
    // Check the password.
    passLen = Reader_ReadByte(msgReader);
    memset(password, 0, sizeof(password));
    Reader_Read(msgReader, password, passLen);
    if(strcmp(password, netPassword))
    {
        Sv_SendText(netBuffer.player, SV_CONSOLE_FLAGS, "Sv_Login: Invalid password.\n");
        return;
    }

    // OK!
    netRemoteUser = netBuffer.player;
    Con_Message("Sv_Login: %s (client %i) logged in.\n",
                clients[netRemoteUser].name, netRemoteUser);
    // Send a confirmation packet to the client.
    Msg_Begin(PKT_LOGIN);
    Writer_WriteByte(msgWriter, true);        // Yes, you're logged in.
    Msg_End();
    Net_SendBuffer(netRemoteUser, 0);
}

/**
 * Executes the command in the message buffer.
 * Usually sent by Con_Send.
 */
void Sv_ExecuteCommand(void)
{
    int flags;
    byte cmdSource;
    unsigned short len;
    boolean silent;
    char *cmd = 0;

    if(!netRemoteUser)
    {
        Con_Message("Sv_ExecuteCommand: Cmd received but no one's logged in!\n");
        return;
    }
    // The command packet is very simple.
    len = Reader_ReadUInt16(msgReader);
    silent = (len & 0x8000) != 0;
    len &= 0x7fff;
    switch(netBuffer.msg.type)
    {
    /*case PKT_COMMAND:
        cmdSource = CMDS_UNKNOWN; // unknown command source.
        break;*/

    case PKT_COMMAND2:
        // New format includes flags and command source.
        // Flags are currently unused but added for future expansion.
        flags = Reader_ReadUInt16(msgReader);
        cmdSource = Reader_ReadByte(msgReader);
        break;

    default:
        Con_Error("Sv_ExecuteCommand: Not a command packet!\n");
        return;
    }

    // Make a copy of the command.
    cmd = M_Malloc(len + 1);
    Reader_Read(msgReader, cmd, len);
    cmd[len] = 0;

    Con_Execute(cmdSource, cmd, silent, true);

    M_Free(cmd);
}

/**
 * Server's packet handler.
 */
void Sv_GetPackets(void)
{
    int         netconsole;
    client_t   *sender;

    while(Net_GetPacket())
    {
        Msg_BeginRead();
        switch(netBuffer.msg.type)
        {
        case PCL_GOODBYE:
            // The client is leaving.
            N_TerminateClient(netBuffer.player);
            break;

        case PKT_COORDS:
            Sv_ClientCoords(netBuffer.player);
            break;

        case PCL_ACK_SHAKE:
            // The client has acknowledged our handshake.
            // Note the time (this isn't perfectly accurate, though).
            netconsole = netBuffer.player;
            if(netconsole >= 0 && netconsole < DDMAXPLAYERS)
            {
                sender = &clients[netconsole];
                sender->shakePing = Sys_GetRealTime() - sender->shakePing;
                Con_Printf("Cl%i handshake ping: %i ms\n", netconsole,
                           sender->shakePing);

                // Update the initial ack time accordingly. Since the ping
                // fluctuates, assume the a poor case.
                Net_SetInitialAckTime(netconsole, 2 * sender->shakePing);
            }
            break;

        case PCL_ACK_PLAYER_FIX:
        {
            player_t               *plr = &ddPlayers[netBuffer.player];
            ddplayer_t             *ddpl = &plr->shared;
            fixcounters_t          *acked = &ddpl->fixAcked;

            acked->angles = Reader_ReadInt32(msgReader);
            acked->pos = Reader_ReadInt32(msgReader);
            acked->mom = Reader_ReadInt32(msgReader);
#ifdef _DEBUG
            Con_Message("PCL_ACK_PLAYER_FIX: (%i) Angles %i (%i), pos %i (%i), mom %i (%i).\n",
                        netBuffer.player,
                        acked->angles,
                        ddpl->fixCounter.angles,
                        acked->pos,
                        ddpl->fixCounter.pos,
                        acked->mom,
                        ddpl->fixCounter.mom);
#endif
            break;
        }

        case PKT_PING:
            Net_PingResponse();
            break;

        case PCL_HELLO:
        case PCL_HELLO2:
        case PKT_OK:
        case PKT_CHAT:
        case PKT_PLAYER_INFO:
            Sv_HandlePacket();
            break;

        case PKT_LOGIN:
            Sv_Login();
            break;

        //case PKT_COMMAND:
        case PKT_COMMAND2:
            Sv_ExecuteCommand();
            break;

        default:
            if(netBuffer.msg.type >= PKT_GAME_MARKER)
            {
                // A client has sent a game specific packet.
                gx.HandlePacket(netBuffer.player, netBuffer.msg.type,
                                netBuffer.msg.data, netBuffer.length);
            }
            break;
        }
        Msg_EndRead();
    }
}

/**
 * Assign a new console to the player. Returns true if successful.
 * Called by N_Update().
 */
boolean Sv_PlayerArrives(unsigned int nodeID, char *name)
{
    int                 i;

    Con_Message("Sv_PlayerArrives: '%s' has arrived.\n", name);

    // We need to find the new player a client entry.
    for(i = 1; i < DDMAXPLAYERS; ++i)
    {
        client_t           *cl = &clients[i];

        if(!cl->connected)
        {
            player_t           *plr = &ddPlayers[i];
            ddplayer_t         *ddpl = &plr->shared;

            // This'll do.
            cl->connected = true;
            cl->ready = false;
            cl->nodeID = nodeID;
            cl->viewConsole = i;
            cl->lastTransmit = -1;
            strncpy(cl->name, name, PLAYERNAMELEN);

            ddpl->fixAcked.angles =
                ddpl->fixAcked.pos =
                ddpl->fixAcked.mom = -1;

            Sv_InitPoolForClient(i);
            Smoother_Clear(cl->smoother);

            VERBOSE(Con_Printf
                    ("Sv_PlayerArrives: '%s' assigned to "
                     "console %i (node: %x)\n", cl->name, i, nodeID));

            // In order to get in the game, the client must first
            // shake hands. It'll request this by sending a Hello packet.
            // We'll be waiting...
            cl->handshake = false;
            //cl->updateCount = UPDATECOUNT;
            return true;
        }
    }

    return false;
}

/**
 * Remove the specified player from the game. Called by N_Update().
 */
void Sv_PlayerLeaves(unsigned int nodeID)
{
    int                 i, plrNum = -1;
    boolean             wasInGame;
    player_t           *plr;
    client_t           *cl;

    // First let's find out who this node actually is.
    for(i = 0; i < DDMAXPLAYERS; ++i)
        if(clients[i].nodeID == nodeID)
        {
            plrNum = i;
            break;
        }

    if(plrNum == -1)
        return; // Bogus?

    // Log off automatically.
    if(netRemoteUser == plrNum)
        netRemoteUser = 0;

    cl = &clients[plrNum];
    plr = &ddPlayers[plrNum];

    // Print a little something in the console.
    Con_Message("Sv_PlayerLeaves: '%s' (console %i) has left, was connected for %f seconds.\n",
                cl->name, plrNum, Sys_GetRealSeconds() - cl->enterTime);

    wasInGame = plr->shared.inGame;
    plr->shared.inGame = false;
    cl->connected = false;
    cl->ready = false;
    //cl->updateCount = 0;
    cl->handshake = false;
    cl->nodeID = 0;
    cl->bandwidthRating = BWR_DEFAULT;

    // Set a modest ack time by default.
    Net_SetInitialAckTime(plrNum, ACK_DEFAULT);

    // Remove the player's data from the register.
    Sv_PlayerRemoved(plrNum);

    if(wasInGame)
    {
        // Inform the DLL about this.
        gx.NetPlayerEvent(plrNum, DDPE_EXIT, NULL);

        // Inform other clients about this.
        Msg_Begin(PSV_PLAYER_EXIT);
        Writer_WriteByte(msgWriter, plrNum);
        Msg_End();
        Net_SendBuffer(NSP_BROADCAST, 0);
    }

    // This client no longer has an ID number.
    cl->id = 0;
}

/**
 * The player will be sent the introductory handshake packets.
 */
void Sv_Handshake(int plrNum, boolean newPlayer)
{
    int i;
    uint playersInGame = 0;

#ifdef _DEBUG
    Con_Message("Sv_Handshake: Shaking hands with player %i.\n", plrNum);
#endif

    for(i = 0; i < DDMAXPLAYERS; ++i)
        if(clients[i].connected)
            playersInGame |= 1 << i;

    Msg_Begin(PSV_HANDSHAKE);
    Writer_WriteByte(msgWriter, SV_VERSION);
    Writer_WriteByte(msgWriter, plrNum);
    Writer_WriteUInt32(msgWriter, playersInGame);
    Writer_WriteFloat(msgWriter, gameTime);
    Msg_End();
    Net_SendBuffer(plrNum, 0);

    //shake.version = SV_VERSION; // byte
    //shake.yourConsole = plrNum; // byte
    //shake.playerMask = 0;
    //shake.gameTime = LONG(gameTime * 100);

    //shake.playerMask = USHORT(shake.playerMask);
    /*Net_SendPacket(plrNum | DDSP_ORDERED, PSV_HANDSHAKE, &shake,
                   sizeof(shake));*/

    if(newPlayer)
    {
        // Note the time when the handshake was sent.
        clients[plrNum].shakePing = Sys_GetRealTime();
    }

    // The game DLL wants to shake hands as well?
    gx.NetWorldEvent(DDWE_HANDSHAKE, plrNum, (void *) &newPlayer);

    // Propagate client information.
    for(i = 0; i < DDMAXPLAYERS; ++i)
    {
        if(clients[i].connected)
        {
            Net_SendPlayerInfo(i, plrNum);
        }

        // Send the new player's info to other players.
        if(newPlayer && i != 0 && i != plrNum && clients[i].connected)
        {
            Net_SendPlayerInfo(plrNum, i);
        }
    }

    if(!newPlayer)
    {
        // This is not a new player (just a re-handshake) but we'll
        // nevertheless re-init the client's state register. For new
        // players this is done in Sv_PlayerArrives.
        Sv_InitPoolForClient(plrNum);
    }

    ddPlayers[plrNum].shared.flags |= DDPF_FIXANGLES | DDPF_FIXPOS | DDPF_FIXMOM;
}

void Sv_StartNetGame(void)
{
    int                 i;

    // Reset all the counters and other data.
    for(i = 0; i < DDMAXPLAYERS; ++i)
    {
        client_t           *client = &clients[i];
        player_t           *plr = &ddPlayers[i];
        ddplayer_t         *ddpl = &plr->shared;

        ddpl->inGame = false;
        ddpl->flags &= ~DDPF_CAMERA;

        client->connected = false;
        client->ready = false;
        client->nodeID = 0;
        //client->numTics = 0;
        //client->firstTic = 0;
        client->enterTime = 0;
        //client->runTime = -1;
        client->lastTransmit = -1;
        //client->updateCount = UPDATECOUNT;
        client->fov = 90;
        client->viewConsole = i;
        memset(client->name, 0, sizeof(client->name));
        client->bandwidthRating = BWR_DEFAULT;
        //client->bwrAdjustTime = 0;
        //memset(client->ackTimes, 0, sizeof(client->ackTimes));
        Smoother_Clear(client->smoother);
    }
    gameTime = 0;
    firstNetUpdate = true;
    netRemoteUser = 0;

    // The server is always player number zero.
    consolePlayer = displayPlayer = 0;

    netGame = true;
    isServer = true;
    allowSending = true;

    if(!isDedicated)
    {
        player_t           *plr = &ddPlayers[consolePlayer];
        ddplayer_t         *ddpl = &plr->shared;
        client_t           *cl = &clients[consolePlayer];

        ddpl->inGame = true;
        cl->connected = true;
        cl->ready = true;
        strcpy(cl->name, playerName);
    }
}

void Sv_SendText(int to, int con_flags, char *text)
{
    uint32_t len = MIN_OF(0xffff, strlen(text));

    Msg_Begin(PSV_CONSOLE_TEXT);
    Writer_WriteUInt32(msgWriter, con_flags & ~CBLF_TRANSMIT);
    Writer_WriteUInt16(msgWriter, len);
    Writer_Write(msgWriter, text, len);
    Msg_End();
    Net_SendBuffer(to, 0);
}

/**
 * Asks a client to disconnect. Clients will immediately disconnect
 * after receiving the PSV_SERVER_CLOSE message.
 */
void Sv_Kick(int who)
{
    if(!clients[who].connected)
        return;

    Sv_SendText(who, SV_CONSOLE_FLAGS, "You were kicked out!\n");
    Msg_Begin(PSV_SERVER_CLOSE);
    Msg_End();
    Net_SendBuffer(who, 0);
}

/**
 * Sends player @a plrNum's position, momentum and/or angles override to all
 * clients.
 */
void Sv_SendPlayerFixes(int plrNum)
{
    int                 fixes = 0;
    player_t           *plr = &ddPlayers[plrNum];
    ddplayer_t         *ddpl = &plr->shared;

    if(!(ddpl->flags & (DDPF_FIXANGLES | DDPF_FIXPOS | DDPF_FIXMOM)))
    {
        // Nothing to fix.
        return;
    }

    // Start writing a player fix message.
    Msg_Begin(PSV_PLAYER_FIX);

    // Which player is being fixed?
    Writer_WriteByte(msgWriter, plrNum);

    // Indicate what is included in the message.
    if(ddpl->flags & DDPF_FIXANGLES) fixes |= 1;
    if(ddpl->flags & DDPF_FIXPOS) fixes |= 2;
    if(ddpl->flags & DDPF_FIXMOM) fixes |= 4;

    Writer_WriteUInt32(msgWriter, fixes);
    Writer_WriteUInt16(msgWriter, ddpl->mo->thinker.id);

    // Increment counters.
    if(ddpl->flags & DDPF_FIXANGLES)
    {
        Writer_WriteInt32(msgWriter, ++ddpl->fixCounter.angles);
        Writer_WriteUInt32(msgWriter, ddpl->mo->angle);
        Writer_WriteFloat(msgWriter, ddpl->lookDir);

#ifdef _DEBUG
Con_Message("Sv_SendPlayerFixes: Sent angles (%i): angle=%f lookdir=%f\n",
            ddpl->fixCounter.angles, FIX2FLT(ddpl->mo->angle),
            ddpl->lookDir);
#endif
    }

    if(ddpl->flags & DDPF_FIXPOS)
    {
        Writer_WriteInt32(msgWriter, ++ddpl->fixCounter.pos);
        Writer_WriteFloat(msgWriter, ddpl->mo->pos[VX]);
        Writer_WriteFloat(msgWriter, ddpl->mo->pos[VY]);
        Writer_WriteFloat(msgWriter, ddpl->mo->pos[VZ]);

#ifdef _DEBUG
Con_Message("Sv_SendPlayerFixes: Sent position (%i): %f, %f, %f\n",
            ddpl->fixCounter.pos,
            ddpl->mo->pos[VX], ddpl->mo->pos[VY], ddpl->mo->pos[VZ]);
#endif
    }

    if(ddpl->flags & DDPF_FIXMOM)
    {
        Writer_WriteInt32(msgWriter, ++ddpl->fixCounter.mom);
        Writer_WriteFloat(msgWriter, ddpl->mo->mom[MX]);
        Writer_WriteFloat(msgWriter, ddpl->mo->mom[MY]);
        Writer_WriteFloat(msgWriter, ddpl->mo->mom[MZ]);

#ifdef _DEBUG
Con_Message("Sv_SendPlayerFixes: Sent momentum (%i): %f, %f, %f\n",
            ddpl->fixCounter.mom,
            ddpl->mo->mom[MX], ddpl->mo->mom[MY], ddpl->mo->mom[MZ]);
#endif
    }

    Msg_End();

    // Send the fix message to everyone.
    Net_SendBuffer(DDSP_ALL_PLAYERS, 0);

    ddpl->flags &= ~(DDPF_FIXANGLES | DDPF_FIXPOS | DDPF_FIXMOM);
#ifdef _DEBUG
    Con_Message("Sv_SendPlayerFixes: Cleared FIX flags of player %i.\n", plrNum);
#endif

    // Clear the smoother for this client.
    Smoother_Clear(clients[plrNum].smoother);
}

void Sv_Ticker(timespan_t ticLength)
{
    int                 i;

    // Note last angles for all players.
    for(i = 0; i < DDMAXPLAYERS; ++i)
    {
        player_t *plr = &ddPlayers[i];

        if(!plr->shared.inGame || !plr->shared.mo)
            continue;

        // Update the smoother.
        Smoother_Advance(clients[i].smoother, ticLength);

        if(DD_IsSharpTick())
        {
            plr->shared.lastAngle = plr->shared.mo->angle;
        }

        /*
        if(clients[i].bwrAdjustTime > 0)
        {
            // BWR adjust time tics away.
            clients[i].bwrAdjustTime--;
        }*/

        // Increment counter, send new data.
        Sv_SendPlayerFixes(i);
    }
}

/**
 * @return              The number of players in the game.
 */
int Sv_GetNumPlayers(void)
{
    int                 i, count;

    // Clients can't count.
    if(isClient)
        return 1;

    for(i = count = 0; i < DDMAXPLAYERS; ++i)
    {
        player_t           *plr = &ddPlayers[i];

        if(plr->shared.inGame && plr->shared.mo)
            count++;
    }

    return count;
}

/**
 * @return              The number of connected clients.
 */
int Sv_GetNumConnected(void)
{
    int                 i, count = 0;

    // Clients can't count.
    if(isClient)
        return 1;

    for(i = isDedicated ? 1 : 0; i < DDMAXPLAYERS; ++i)
        if(clients[i].connected)
            count++;

    return count;
}

/**
 * The bandwidth rating is updated according to the status of the
 * player's send queue. Returns true if a new packet may be sent.
 */
boolean Sv_CheckBandwidth(int playerNumber)
{
    return true;
    /*
    client_t           *client = &clients[playerNumber];
    uint                qSize = N_GetSendQueueSize(playerNumber);
    uint                limit = 400;

    return true;

    // If there are too many messages in the queue, the player's bandwidth
    // is overrated.
    if(qSize > limit)
    {
        // Drop quickly to allow the send queue to clear out sooner.
        client->bandwidthRating -= 10;
    }

    // If the send queue is practically empty, we can use more bandwidth.
    // (Providing we have BWR adjust time.)
    if(qSize < limit / 20 && client->bwrAdjustTime > 0)
    {
        client->bandwidthRating++;

        // Increase BWR only once during the adjust time.
        client->bwrAdjustTime = 0;
    }

    // Do not go past the boundaries, though.
    if(client->bandwidthRating < 0)
    {
        client->bandwidthRating = 0;
    }
    if(client->bandwidthRating > MAX_BANDWIDTH_RATING)
    {
        client->bandwidthRating = MAX_BANDWIDTH_RATING;
    }

    // New messages will not be sent if there's too much already.
    return qSize <= 10 * limit;
    */
}

/**
 * Reads a PKT_COORDS packet from the message buffer. We trust the
 * client's position and change ours to match it. The client better not
 * be cheating.
 */
void Sv_ClientCoords(int plrNum)
{
    player_t           *plr = &ddPlayers[plrNum];
    ddplayer_t         *ddpl = &plr->shared;
    mobj_t             *mo = ddpl->mo;
    int                 clz;
    float               clientGameTime;
    float               clientPos[3];
    angle_t             clientAngle;
    float               clientLookDir;
    boolean             onFloor = false;

    // If mobj or player is invalid, the message is discarded.
    if(!mo || !ddpl->inGame || (ddpl->flags & DDPF_DEAD))
        return;

    clientGameTime = Reader_ReadFloat(msgReader);

    clientPos[VX] = Reader_ReadFloat(msgReader);
    clientPos[VY] = Reader_ReadFloat(msgReader);

    clz = Reader_ReadInt32(msgReader);
    if(clz == DDMININT)
    {
        clientPos[VZ] = mo->floorZ;
        onFloor = true;
    }
    else
    {
        clientPos[VZ] = FIX2FLT(clz);
    }

    // The angles.
    clientAngle = ((angle_t) Reader_ReadUInt16(msgReader)) << 16;
    clientLookDir = P_ShortToLookDir(Reader_ReadInt16(msgReader));

    // Movement intent.
    ddpl->forwardMove = FIX2FLT(Reader_ReadChar(msgReader) << 13);
    ddpl->sideMove = FIX2FLT(Reader_ReadChar(msgReader) << 13);

    if(ddpl->fixCounter.angles == ddpl->fixAcked.angles && !(ddpl->flags & DDPF_FIXANGLES))
    {
#ifdef _DEBUG
        VERBOSE2( Con_Message("Sv_ClientCoords: Setting angles for player %i: %x, %f\n", plrNum, clientAngle, clientLookDir) );
#endif
        mo->angle = clientAngle;
        ddpl->lookDir = clientLookDir;
    }

    /*
    if(ddpl->fixCounter.mom == ddpl->fixAcked.mom && !(ddpl->flags & DDPF_FIXMOM))
    {
#ifdef _DEBUG
        VERBOSE2( Con_Message("Sv_ClientCoords: Setting momentum for player %i: %f, %f, %f\n", plrNum,
                              clientMom[VX], clientMom[VY], clientMom[VZ]) );
#endif
        mo->mom[VX] = clientMom[VX];
        mo->mom[VY] = clientMom[VY];
        mo->mom[VZ] = clientMom[VZ];
    }
    */

#ifdef _DEBUG
    VERBOSE2( Con_Message("Sv_ClientCoords: Received coords for player %i: %f, %f, %f\n", plrNum,
                          clientPos[VX], clientPos[VY], clientPos[VZ]) );
#endif

    // If we aren't about to forcibly change the client's position, update
    // with new pos if it's valid. But it must be a valid pos.
    if(Sv_CanTrustClientPos(plrNum))
    {
#ifdef _DEBUG
        VERBOSE2( Con_Message("Sv_ClientCoords: Setting coords for player %i: %f, %f, %f\n", plrNum,
                              clientPos[VX], clientPos[VY], clientPos[VZ]) );
#endif
        Smoother_AddPos(clients[plrNum].smoother, clientGameTime,
                        clientPos[VX], clientPos[VY], clientPos[VZ], onFloor);
    }
}

/**
 * Determines whether the coordinates sent by a player are valid at the moment.
 */
boolean Sv_CanTrustClientPos(int plrNum)
{
    player_t* plr = &ddPlayers[plrNum];
    ddplayer_t* ddpl = &plr->shared;

    if(ddpl->fixCounter.pos == ddpl->fixAcked.pos && !(ddpl->flags & DDPF_FIXPOS))
    {
        return true;
    }
    // Server's position is valid, client is not up-to-date.
    return false;
}

/**
 * Console command for terminating a remote console connection.
 */
D_CMD(Logout)
{
    // Only servers can execute this command.
    if(!netRemoteUser || !isServer)
        return false;
    // Notice that the server WILL execute this command when a client
    // is logged in and types "logout".
    Sv_SendText(netRemoteUser, SV_CONSOLE_FLAGS, "Goodbye...\n");
    // Send a logout packet.
    Msg_Begin(PKT_LOGIN);
    Writer_WriteByte(msgWriter, false);       // You're outta here.
    Msg_End();
    Net_SendBuffer(netRemoteUser, 0);
    netRemoteUser = 0;
    return true;
}
