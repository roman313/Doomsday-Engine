/**\file
 *\section Copyright and License Summary
 * License: GPL + jHeretic/jHexen Exception
 * Online License Link: http://www.gnu.org/licenses/gpl.html
 *
 *\author Copyright © 2004-2006 Jaakko Keränen <skyjake@dengine.net>
 *\author Copyright © 2005-2006 Daniel Swanson <danij@dengine.net>
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
 *
 * In addition, as a special exception, we, the authors of deng
 * give permission to link the code of our release of deng with
 * the libjhexen and/or the libjheretic libraries (or with modified
 * versions of it that use the same license as the libjhexen or
 * libjheretic libraries), and distribute the linked executables.
 * You must obey the GNU General Public License in all respects for
 * all of the code used other than “libjhexen or libjheretic”. If
 * you modify this file, you may extend this exception to your
 * version of the file, but you are not obligated to do so. If you
 * do not wish to do so, delete this exception statement from your version.
 */

#ifndef __H2DEF__
#define __H2DEF__

#ifndef __JHEXEN__
#  error "Using jHexen headers without __JHEXEN__"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "doomsday.h"
#include "dd_api.h"
#include "g_dgl.h"
#include "version.h"

#define Set DD_SetInteger
#define Get DD_GetInteger

#define CONFIGFILE    GAMENAMETEXT".cfg"
#define DEFSFILE      GAMENAMETEXT"\\"GAMENAMETEXT".ded"
#define DATAPATH      "}data\\"GAMENAMETEXT"\\"
#define STARTUPWAD    "}data\\"GAMENAMETEXT"\\"GAMENAMETEXT".wad"
#define STARTUPPK3    "}data\\"GAMENAMETEXT"\\"GAMENAMETEXT".pk3"

// Misc macros.
#define CLAMP(v, min, max) (v < min? v=min : v > max? v=max : v)

#ifdef WIN32
#pragma warning (disable:4244)
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

// Uncomment, to enable all timebomb stuff
//#define TIMEBOMB
#define TIMEBOMB_YEAR   95         // years since 1900
#define TIMEBOMB_STARTDATE  268    // initial date (9/26)
#define TIMEBOMB_ENDDATE    301    // end date (10/29)


// all exterior data is defined here
#include "xddefs.h"

// all important printed strings
#include "textdefs.h"

// header generated by multigen utility
#include "info.h"

#define FINEANGLES          8192
#define FINEMASK            (FINEANGLES-1)
#define ANGLETOFINESHIFT    19     // 0x100000000 to 0x2000

#define MAXPLAYERS      8
#define BACKUPTICS      12

enum { VX, VY, VZ };

extern game_import_t gi;

#define states      (*gi.states)
#define mobjinfo    (*gi.mobjinfo)

/*
   ===============================================================================

   GLOBAL TYPES

   ===============================================================================
 */

typedef enum {
    SM_BABY,
    SM_EASY,
    SM_MEDIUM,
    SM_HARD,
    SM_NIGHTMARE,
    NUM_SKILL_MODES
} skillmode_t;

// Game mode handling - identify IWAD version
//  to handle IWAD dependend animations etc.
typedef enum {
    shareware,                     // 4 level demo
    registered,                    // HEXEN registered
    extended,                      // DeathKings
    indetermined,                  // Well, no IWAD found.
    NUM_GAME_MODES
} gamemode_t;

// Game mode bits for the above.
#define GM_SHAREWARE        0x1    // 4 level demo
#define GM_REGISTERED       0x2    // HEXEN registered
#define GM_EXTENDED         0x4    // DeathKings
#define GM_INDETERMINED     0x8    // Well, no IWAD found.

#define GM_ANY              (GM_SHAREWARE|GM_REGISTERED|GM_EXTENDED)
#define GM_NOTSHAREWARE     (GM_REGISTERED|GM_EXTENDED)

typedef enum {
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN,
    GS_WAITING,
    GS_INFINE
} gamestate_t;

typedef enum {
    wipe_0,
    wipe_1,
    wipe_2,
    wipe_3,
    wipe_4,
    NUMWIPES,
    wipe_random
} wipe_t;

/*
   ===============================================================================

   MAPOBJ DATA

   ===============================================================================
 */

struct player_s;

typedef struct mobj_s {
    // Defined in dd_share.h; required mobj elements.
    DD_BASE_MOBJ_ELEMENTS()
        // Hexen-specific data:
    struct player_s *player;       // only valid if type == MT_PLAYER

    fixed_t         floorpic;      // contacted sec floorpic
    mobjinfo_t     *info;          // &mobjinfo[mobj->type]
    int             damage;        // For missiles
    int             flags;
    int             flags2;        // Heretic flags
    int             flags3;
    int             special1;      // Special info
    int             special2;      // Special info
    int             health;
    int             movedir;       // 0-7
    int             movecount;     // when 0, select a new dir
    struct mobj_s  *target;        // thing being chased/attacked (or NULL)
    // also the originator for missiles
    // used by player to freeze a bit after
    // teleporting
    int             threshold;     // if > 0, the target will be chased
    // no matter what (even if shot)
    int             lastlook;      // player number last looked for
    int             archiveNum;    // Identity during archive
    short           tid;           // thing identifier
    byte            special;       // special
    byte            args[5];       // special arguments
    int             turntime;      // $visangle-facetarget
    int             alpha;         // $mobjalpha

    // Thing being chased/attacked for tracers.
    struct mobj_s  *tracer;

    // Used by lightning zap
    struct mobj_s  *lastenemy;
} mobj_t;

// each sector has a degenmobj_t in it's center for sound origin purposes

// Most damage defined using HITDICE
#define HITDICE(a) ((1+(P_Random()&7))*a)

//
// frame flags
//

// --- mobj.flags ---

#define MF_SPECIAL      1          // call P_SpecialThing when touched
#define MF_SOLID        2
#define MF_SHOOTABLE    4
#define MF_NOSECTOR     8          // don't use the sector links
                                    // (invisible but touchable)
#define MF_NOBLOCKMAP   16         // don't use the blocklinks
                                    // (inert but displayable)
#define MF_AMBUSH       32
#define MF_JUSTHIT      64         // try to attack right back
#define MF_JUSTATTACKED 128        // take at least one step before attacking
#define MF_SPAWNCEILING 256        // hang from ceiling instead of floor
#define MF_NOGRAVITY    512        // don't apply gravity every tic

// movement flags
#define MF_DROPOFF      0x400      // allow jumps from high places
#define MF_PICKUP       0x800      // for players to pick up items
#define MF_NOCLIP       0x1000     // player cheat
#define MF_SLIDE        0x2000     // keep info about sliding along walls
#define MF_FLOAT        0x4000     // allow moves to any height, no gravity
#define MF_TELEPORT     0x8000     // don't cross lines or look at heights
#define MF_MISSILE      0x10000    // don't hit same species, explode on block

#define MF_ALTSHADOW    0x20000    // alternate fuzzy draw
#define MF_SHADOW       0x40000    // use fuzzy draw (shadow demons / invis)
#define MF_NOBLOOD      0x80000    // don't bleed when shot (use puff)
#define MF_CORPSE       0x100000   // don't stop moving halfway off a step
#define MF_INFLOAT      0x200000   // floating to a height for a move, don't
                                    // auto float to target's height

#define MF_COUNTKILL    0x400000   // count towards intermission kill total
#define MF_ICECORPSE    0x800000   // a frozen corpse (for blasting)

#define MF_SKULLFLY     0x1000000  // skull in flight
#define MF_NOTDMATCH    0x2000000  // don't spawn in death match (key cards)

//#define   MF_TRANSLATION  0xc000000   // if 0x4 0x8 or 0xc, use a translation
#define MF_TRANSLATION  0x1c000000 // use a translation table (>>MF_TRANSHIFT)
#define MF_TRANSSHIFT   26         // table for player colormaps

#define MF_LOCAL            0x20000000

// If this flag is set, the sprite is aligned with the view plane.
#define MF_BRIGHTEXPLODE    0x40000000  // Make this brightshadow when exploding.
#define MF_VIEWALIGN        0x80000000
#define MF_BRIGHTSHADOW     (MF_SHADOW|MF_ALTSHADOW)

// --- mobj.flags2 ---

#define MF2_LOGRAV          0x00000001  // alternate gravity setting
#define MF2_WINDTHRUST      0x00000002  // gets pushed around by the wind
                                        // specials
#define MF2_FLOORBOUNCE     0x00000004  // bounces off the floor
#define MF2_BLASTED         0x00000008  // missile will pass through ghosts
#define MF2_FLY             0x00000010  // fly mode is active
#define MF2_FLOORCLIP       0x00000020  // if feet are allowed to be clipped
#define MF2_SPAWNFLOAT      0x00000040  // spawn random float z
#define MF2_NOTELEPORT      0x00000080  // does not teleport
#define MF2_RIP             0x00000100  // missile rips through solid
                                        // targets
#define MF2_PUSHABLE        0x00000200  // can be pushed by other moving
                                        // mobjs
#define MF2_SLIDE           0x00000400  // slides against walls
#define MF2_ONMOBJ          0x00000800  // mobj is resting on top of another
                                        // mobj
#define MF2_PASSMOBJ        0x00001000  // Enable z block checking.  If on,
                                        // this flag will allow the mobj to
                                        // pass over/under other mobjs.
#define MF2_CANNOTPUSH      0x00002000  // cannot push other pushable mobjs
#define MF2_DROPPED         0x00004000  // dropped by a demon
#define MF2_BOSS            0x00008000  // mobj is a major boss
#define MF2_FIREDAMAGE      0x00010000  // does fire damage
#define MF2_NODMGTHRUST     0x00020000  // does not thrust target when
                                        // damaging
#define MF2_TELESTOMP       0x00040000  // mobj can stomp another
#define MF2_FLOATBOB        0x00080000  // use float bobbing z movement
#define MF2_DONTDRAW        0x00100000  // don't generate a vissprite
#define MF2_IMPACT          0x00200000  // an MF_MISSILE mobj can activate
                                        // SPAC_IMPACT
#define MF2_PUSHWALL        0x00400000  // mobj can push walls
#define MF2_MCROSS          0x00800000  // can activate monster cross lines
#define MF2_PCROSS          0x01000000  // can activate projectile cross lines
#define MF2_CANTLEAVEFLOORPIC 0x02000000    // stay within a certain floor type
#define MF2_NONSHOOTABLE    0x04000000  // mobj is totally non-shootable,
                                        // but still considered solid
#define MF2_INVULNERABLE    0x08000000  // mobj is invulnerable
#define MF2_DORMANT         0x10000000  // thing is dormant
#define MF2_ICEDAMAGE       0x20000000  // does ice damage
#define MF2_SEEKERMISSILE   0x40000000  // is a seeker (for reflection)
#define MF2_REFLECTIVE      0x80000000  // reflects missiles

// --- mobj.flags3 ---

#define MF3_NOINFIGHT       0x00000001  // Mobj will never be targeted for in-fighting

//=============================================================================

typedef enum {
    ARMOR_ARMOR,
    ARMOR_SHIELD,
    ARMOR_HELMET,
    ARMOR_AMULET,
    NUMARMOR
} armortype_t;

// ===== Player Class Types =====
typedef enum {
    PCLASS_FIGHTER,
    PCLASS_CLERIC,
    PCLASS_MAGE,
    PCLASS_PIG,
    NUM_PLAYER_CLASSES
} playerclass_t;

#define PCLASS_INFO(class)  (&classInfo[class])

typedef struct classinfo_s{
    mobjtype_t  mobjtype;
    int         normalstate;
    int         runstate;
    int         attackstate;
    int         attackendstate;
    int         maxarmor;
    int         autoarmorsave;
    fixed_t     maxmove;
    fixed_t     forwardmove[2];     // walk, run
    fixed_t     sidemove[2];        // walk, run
    int         movemul;            // multiplier for above
    fixed_t     turnSpeed[3];       // [normal, speed, initial]
    int         jumptics;           // wait inbetween jumps
    int         failUseSound;       // sound played when a use fails.
    int         armorincrement[NUMARMOR];
    int         piecex[3];          // temp
} classinfo_t;

extern classinfo_t classInfo[NUM_PLAYER_CLASSES];

typedef enum {
    KT_KEY1,
    KT_KEY2,
    KT_KEY3,
    KT_KEY4,
    KT_KEY5,
    KT_KEY6,
    KT_KEY7,
    KT_KEY8,
    KT_KEY9,
    KT_KEYA,
    KT_KEYB,
    NUM_KEY_TYPES
} keytype_t;

typedef enum {
    WT_FIRST,
    WT_SECOND,
    WT_THIRD,
    WT_FOURTH,
    NUM_WEAPON_TYPES,

    WT_NOCHANGE
} weapontype_t;

#define NUMWEAPLEVELS 1

typedef enum {
    AT_BLUEMANA,
    AT_GREENMANA,
    NUM_AMMO_TYPES,

    AT_NOAMMO
} ammotype_t;

#define MAX_MANA    200

#define WPIECE1     1
#define WPIECE2     2
#define WPIECE3     4

#define WEAPON_INFO(weaponnum, pclass, fmode) (&weaponinfo[weaponnum][pclass].mode[fmode])

typedef struct {
    int             gamemodebits;  // Game modes, weapon is available in.
    int             ammotype[NUM_AMMO_TYPES];  // required ammo types.
    int             pershot[NUM_AMMO_TYPES];   // Ammo used per shot of each type.
    boolean         autofire;           // (True)= fire when raised if fire held.
    int             upstate;
    int             raisesound;         // Sound played when weapon is raised.
    int             downstate;
    int             readystate;
    int             readysound;         // Sound played WHILE weapon is readyied.
    int             atkstate;
    int             holdatkstate;
    int             flashstate;
} weaponmodeinfo_t;

typedef struct {
    weaponmodeinfo_t mode[NUMWEAPLEVELS];
} weaponinfo_t;

extern weaponinfo_t weaponinfo[NUM_WEAPON_TYPES][NUM_PLAYER_CLASSES];

typedef enum {
    arti_none,
    arti_invulnerability,
    arti_health,
    arti_superhealth,
    arti_healingradius,
    arti_summon,
    arti_torch,
    arti_egg,
    arti_fly,
    arti_blastradius,
    arti_poisonbag,
    arti_teleportother,
    arti_speed,
    arti_boostmana,
    arti_boostarmor,
    arti_teleport,
    // Puzzle artifacts
    arti_firstpuzzitem,
    arti_puzzskull = arti_firstpuzzitem,
    arti_puzzgembig,
    arti_puzzgemred,
    arti_puzzgemgreen1,
    arti_puzzgemgreen2,
    arti_puzzgemblue1,
    arti_puzzgemblue2,
    arti_puzzbook1,
    arti_puzzbook2,
    arti_puzzskull2,
    arti_puzzfweapon,
    arti_puzzcweapon,
    arti_puzzmweapon,
    arti_puzzgear1,
    arti_puzzgear2,
    arti_puzzgear3,
    arti_puzzgear4,
    NUMARTIFACTS
} artitype_e;

#define MAXARTICOUNT 25

typedef enum {
    PT_NONE,
    PT_INVULNERABILITY,
    PT_ALLMAP,
    PT_INFRARED,
    PT_FLIGHT,
    PT_SHIELD,
    PT_HEALTH2,
    PT_SPEED,
    PT_MINOTAUR,
    NUM_POWER_TYPES
} powertype_t;

#define INVULNTICS (30*35)
#define INVISTICS (60*35)
#define INFRATICS (120*35)
#define IRONTICS (60*35)
#define WPNLEV2TICS (40*35)
#define FLIGHTTICS (60*35)
#define SPEEDTICS (45*35)
#define MORPHTICS (40*35)
//#define MAULATORTICS (25*35)

#define BLINKTHRESHOLD (4*35)

extern int      MaulatorSeconds;

#define MAULATORTICS    ( (unsigned int) /*((netgame || demorecording || demoplayback)? 25*35 :*/ MaulatorSeconds*35 )

#define NUMINVENTORYSLOTS   NUMARTIFACTS

typedef struct {
    int             type;
    int             count;
} inventory_t;

/*
   ================
   =
   = player_t
   =
   ================
 */
#if 0
// Now unused? Don't we support HEXEN.EXE 1.1 save games?
#pragma pack(1)
typedef struct saveplayer_s {
    mobj_t         *mo;
    playerstate_t   playerstate;
    ticcmd_t        cmd;

    playerclass_t        class;         // player class type

    fixed_t         viewz;         // focal origin above r.z
    fixed_t         viewheight;    // base height above floor for viewz
    fixed_t         deltaviewheight;    // squat speed
    fixed_t         bob;           // bounded/scaled total momentum

    int             flyheight;
    //int           lookdir;
    float           lookdir;       // It's now a float, for mlook. -jk
    boolean         centering;
    int             health;        // only used between levels, mo->health
    // is used during levels
    int             armorpoints[NUMARMOR];

    inventory_t     inventory[NUMINVENTORYSLOTS];
    artitype_t      readyArtifact;
    int             artifactCount;
    int             inventorySlotNum;
    int             powers[NUM_POWER_TYPES];
    int             keys;
    int             pieces;        // Fourth Weapon pieces
    signed int      frags[MAXPLAYERS];  // kills of other players
    weapontype_t    readyweapon;
    weapontype_t    pendingweapon; // wp_nochange if not changing
    boolean         weaponowned[NUM_WEAPON_TYPES];
    int             mana[NUMMANA];
    int             attackdown, usedown;    // true if button down last tic
    int             cheats;        // bit flags

    int             refire;        // refired shots are less accurate

    int             killcount, itemcount, secretcount;  // for intermission
    char            *message;   // hint messages
    int             messageTics;   // counter for showing messages
    short           ultimateMessage;
    short           yellowMessage;
    int             damagecount, bonuscount;    // for screen flashing
    int             poisoncount;   // screen flash for poison damage
    mobj_t         *poisoner;      // NULL for non-player mobjs
    mobj_t         *attacker;      // who did damage (NULL for floors)
    int             extralight;    // so gun flashes light up areas
    int             fixedcolormap; // can be set to REDCOLORMAP, etc
    int             colormap;      // 0-3 for which color to draw player
    pspdef_t        psprites[NUMPSPRITES];  // view sprites (gun, etc)
    int             morphTics;     // player is a pig if > 0
    uint            jumptics;      // delay the next jump for a moment
    unsigned int    worldTimer;    // total time the player's been playing
} saveplayer_t;
#pragma pack()
#endif

#define SBARHEIGHT  39             // status bar height at bottom of screen

/*
   ===============================================================================

   GLOBAL VARIABLES

   ===============================================================================
 */

#define TELEFOGHEIGHT (32*FRACUNIT)

extern fixed_t  finesine[5 * FINEANGLES / 4];
extern fixed_t *finecosine;

extern boolean  DevMaps;           // true = map development mode
extern char    *DevMapsDir;        // development maps directory

#define VERBOSE(code)   { if(verbose >= 1) { code; } }
#define VERBOSE2(code)  { if(verbose >= 2) { code; } }


#define IS_NETGAME   Get(DD_NETGAME)

extern boolean  cmdfrag;           // true if a CMD_FRAG packet should be sent out every

//extern boolean playeringame[MAXPLAYERS];
//extern playerclass_t PlayerClass[MAXPLAYERS];
//extern byte PlayerColor[MAXPLAYERS];

//extern int consoleplayer; // player taking events and displaying
#define consoleplayer Get(DD_CONSOLEPLAYER)

//extern int displayplayer;
#define displayplayer Get(DD_DISPLAYPLAYER)

extern int      DebugSound;        // debug flag for displaying sound info

//extern boolean demoplayback;
extern int      maxzone;           // Maximum chunk allocated for zone heap

extern int      Sky1Texture;
extern int      Sky2Texture;

extern int      prevmap;

#define MAX_PLAYER_STARTS 8

extern byte    *memscreen;         // off screen work buffer, from V_video.c

/*
   ===============================================================================

   GLOBAL FUNCTIONS

   ===============================================================================
 */

//-----------
//MEMORY ZONE
// tags < 100 are not overwritten until freed
//----------

//BASE LEVEL
//----------
void            H2_Main(void);

// not a globally visible function, just included for source reference
// calls all startup code
// parses command line options

void            G_IdentifyVersion(void);
void            R_SetFilter(int filter);
int             R_GetFilterColor(int filter);

int             G_GetInteger(int id);
void           *G_GetVariable(int id);

//----
//GAME
//----

void            G_DeathMatchSpawnPlayer(int playernum);

int             G_GetLevelNumber(int episode, int map);

void            G_InitNew(skillmode_t skill, int episode, int map);

void            G_DeferedInitNew(skillmode_t skill, int episode, int map);

// can be called by the startup code or M_Responder
// a normal game starts at map 1, but a warp test can start elsewhere

void            G_DeferredNewGame(skillmode_t skill);

void            G_DeferedPlayDemo(char *demo);
void            G_DoPlayDemo(void);

void            G_LoadGame(int slot);

// can be called by the startup code or M_Responder
// calls P_SetupLevel or W_EnterWorld
void            G_DoLoadGame(void);

void            G_SaveGame(int slot, char *description);

// called by M_Responder

void            G_RecordDemo(skillmode_t skill, int numplayers, int episode,
                             int map, char *name);
// only called by startup code

void            G_PlayDemo(char *name);
void            G_TimeDemo(char *name);

void            G_TeleportNewMap(int map, int position);

void            G_LeaveLevel(int map, int position, boolean secret);

void            G_StartNewGame(skillmode_t skill);
void            G_StartNewInit(void);

void            G_WorldDone(void);

void            G_Ticker(void);
boolean         G_Responder(event_t *ev);

void            G_ScreenShot(void);

void            G_DoReborn(int playernum);
void            G_StopDemo(void);
void            G_DemoEnds(void);
void            G_DemoAborted(void);

//-----
//PLAY
//-----

struct xline_s *P_XLine(line_t* line);
struct xsector_s *P_XSector(sector_t* sector);
struct xsector_s* P_XSectorOfSubsector(subsector_t* sub);


// called by C_Ticker
// can call G_PlayerExited
// carries out all thinking of monsters and players

void            P_SetupLevel(int episode, int map, int playermask,
                             skillmode_t skill);

// called by W_Ticker

void            P_Init(void);

//-------
//REFRESH
//-------

extern boolean  setsizeneeded;

void            R_SetViewSize(int blocks, int detail);

// called by M_Responder

extern int      localQuakeHappening[MAXPLAYERS];

//int M_Random (void);
// returns a number from 0 to 255

#ifdef TIC_DEBUG

extern FILE    *rndDebugfile;

#define FUNTAG(fun) { if(rndDebugfile) fprintf(rndDebugfile, "%i: %s\n", gametic, fun); }

#endif                          // TIC_DEBUG

//returns a number from 0 to 255
byte            P_Random(void);
void            M_ClearRandom(void);

extern unsigned char rndtable[256];

//------------------------------
// SC_man.c
//------------------------------

void            SC_Open(char *name);
void            SC_OpenLump(char *name);
void            SC_OpenFile(char *name);
void            SC_OpenFileCLib(char *name);
void            SC_Close(void);
boolean         SC_GetString(void);
void            SC_MustGetString(void);
void            SC_MustGetStringName(char *name);
boolean         SC_GetNumber(void);
void            SC_MustGetNumber(void);
void            SC_UnGet(void);

//boolean SC_Check(void);
boolean         SC_Compare(char *text);
int             SC_MatchString(char **strings);
int             SC_MustMatchString(char **strings);
void            SC_ScriptError(char *message);

extern char    *sc_String;
extern int      sc_Number;
extern int      sc_Line;
extern boolean  sc_End;
extern boolean  sc_Crossed;
extern boolean  sc_FileScripts;
extern char    *sc_ScriptsDir;

//------------------------------
// SN_sonix.c
//------------------------------

enum {
    SEQ_PLATFORM,
    SEQ_PLATFORM_HEAVY,            // same script as a normal platform
    SEQ_PLATFORM_METAL,
    SEQ_PLATFORM_CREAK,            // same script as a normal platform
    SEQ_PLATFORM_SILENCE,
    SEQ_PLATFORM_LAVA,
    SEQ_PLATFORM_WATER,
    SEQ_PLATFORM_ICE,
    SEQ_PLATFORM_EARTH,
    SEQ_PLATFORM_METAL2,
    SEQ_DOOR_STONE,
    SEQ_DOOR_HEAVY,
    SEQ_DOOR_METAL,
    SEQ_DOOR_CREAK,
    SEQ_DOOR_SILENCE,
    SEQ_DOOR_LAVA,
    SEQ_DOOR_WATER,
    SEQ_DOOR_ICE,
    SEQ_DOOR_EARTH,
    SEQ_DOOR_METAL2,
    SEQ_ESOUND_WIND,
    SEQ_NUMSEQ
};

typedef enum {
    SEQTYPE_STONE,
    SEQTYPE_HEAVY,
    SEQTYPE_METAL,
    SEQTYPE_CREAK,
    SEQTYPE_SILENCE,
    SEQTYPE_LAVA,
    SEQTYPE_WATER,
    SEQTYPE_ICE,
    SEQTYPE_EARTH,
    SEQTYPE_METAL2,
    SEQTYPE_NUMSEQ
} seqtype_t;

void            SN_InitSequenceScript(void);
void            SN_StartSequence(mobj_t *mobj, int sequence);
void            SN_StartSequenceInSec(sector_t *sector, int seqBase);
void            SN_StartSequenceName(mobj_t *mobj, char *name);
void            SN_StopSequence(mobj_t *mobj);
void            SN_StopSequenceInSec(sector_t *sector);
void            SN_UpdateActiveSequences(void);
void            SN_StopAllSequences(void);
int             SN_GetSequenceOffset(int sequence, int *sequencePtr);
void            SN_ChangeNodeData(int nodeNum, int seqOffset, int delayTics,
                                  int volume, int currentSoundID);

typedef struct seqnode_s seqnode_t;
struct seqnode_s {
    int            *sequencePtr;
    int             sequence;
    mobj_t         *mobj;
    int             currentSoundID;
    int             delayTics;
    int             volume;
    int             stopSound;
    seqnode_t      *prev;
    seqnode_t      *next;
};

extern int      ActiveSequences;
extern seqnode_t *SequenceListHead;

//----------------------
// Interlude (IN_lude.c)
//----------------------

#define MAX_INTRMSN_MESSAGE_SIZE 1024

extern boolean  intermission;
extern char     ClusterMessage[MAX_INTRMSN_MESSAGE_SIZE];

void            IN_Start(void);
void            IN_Stop(void);
void            IN_Ticker(void);
void            IN_Drawer(void);

//----------------------
// Chat mode (CT_chat.c)
//----------------------

void            CT_Init(void);
void            CT_Drawer(void);
boolean         CT_Responder(event_t *ev);
void            CT_Ticker(void);
char            CT_dequeueChatChar(void);

extern boolean  chatmodeon;

#if 0
//--------------------
// Finale (F_finale.c)
//--------------------

void            F_Drawer(void);
void            F_Ticker(void);
void            F_StartFinale(void);
#endif

//----------------------
// STATUS BAR (SB_bar.c)
//----------------------

extern int      SB_state;
void            SB_Init(void);
void            SB_SetClassData(void);
boolean         SB_Responder(event_t *event);
void            SB_Ticker(void);
void            SB_Drawer(int fullscreenmode, boolean refresh);
void            Draw_TeleportIcon(void);
void            Draw_SaveIcon(void);
void            Draw_LoadIcon(void);
void            Draw_BeginZoom(float s, float originX, float originY);
void            Draw_EndZoom(void);

//-----------------
// MENU (MN_menu.c)
//-----------------

void            MN_Init(void);
void            MN_ActivateMenu(void);
void            MN_DeactivateMenu(void);
boolean         M_Responder(event_t *event);
void            MN_Ticker(void);
void            M_Drawer(void);
void            MN_TextFilter(char *text);
void            MN_DrTextA(char *text, int x, int y);
void            MN_DrTextAYellow(char *text, int x, int y);
int             MN_TextAWidth(char *text);
void            MN_DrTextB(char *text, int x, int y);
int             MN_TextBWidth(char *text);
void            MN_DrawTitle(char *text, int y);

// Drawing text in the current state.
void            MN_DrTextA_CS(char *text, int x, int y);
void            MN_DrTextAYellow_CS(char *text, int x, int y);
void            MN_DrTextB_CS(char *text, int x, int y);

// A macro to determine whether it's OK to be backwards incompatible.
//#define INCOMPAT_OK       (!demorecording && !demoplayback && !netgame)

void            strcatQuoted(char *dest, char *src);

// Set if homebrew PWAD stuff has been added.
extern boolean  modifiedgame;

extern byte     gammatable[5][256];

#include "sounds.h"
#include "soundst.h"

#define IS_SERVER       Get(DD_SERVER)
#define IS_CLIENT       Get(DD_CLIENT)
#define IS_NETGAME      Get(DD_NETGAME)
#define IS_DEDICATED    Get(DD_DEDICATED)

#include "g_common.h"

#endif                          // __H2DEF__
