/** @file games.cpp Specialized collection for a set of logical Games.
 *
 * @authors Copyright &copy; 2012-2013 Daniel Swanson <danij@dengine.net>
 * @authors Copyright &copy; 2012-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
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

#include "de_base.h"
#include "de_console.h"
#include "de_filesys.h"
#include "filesys/manifest.h"
#include "resource/zip.h"
#include <QtAlgorithms>

#include "games.h"

namespace de {

DENG2_PIMPL(Games)
{
    /// The actual collection.
    Games::All games;

    /// Special "null-game" object for this collection.
    NullGame *nullGame;

    Instance(Public *i) : Base(i), games(), nullGame(0)
    {
        /*
         * One-time creation and initialization of the special "null-game"
         * object (activated once created).
         */
        nullGame = new NullGame;
    }

    ~Instance()
    {
        clear();
        delete nullGame;
    }

    void clear()
    {
        DENG2_ASSERT(nullGame != 0);

        qDeleteAll(games);
        games.clear();
    }
};

Games::Games() : d(new Instance(this))
{}

Game &Games::nullGame() const
{
    return *d->nullGame;
}

int Games::numPlayable() const
{
    int count = 0;
    foreach(Game *game, d->games)
    {
        if(!game->allStartupFilesFound()) continue;
        ++count;
    }
    return count;
}

Game *Games::firstPlayable() const
{
    foreach(Game *game, d->games)
    {
        if(game->allStartupFilesFound()) return game;
    }
    return NULL;
}

gameid_t Games::id(Game const &game) const
{
    if(&game == d->nullGame) return 0; // Invalid id.
    int idx = d->games.indexOf(const_cast<Game *>(&game));
    if(idx < 0)
    {
        /// @throw NotFoundError  The specified @a game is not a member of the collection.
        throw NotFoundError("Games::id", QString("Game %p is not a member of the collection").arg(de::dintptr(&game)));
    }
    return gameid_t(idx+1);
}

Game &Games::byId(gameid_t gameId) const
{
    if(gameId <= 0 || gameId > d->games.count())
    {
        /// @throw NotFoundError  The specified @a gameId is out of range.
        throw NotFoundError("Games::byId", QString("There is no Game with id %i").arg(gameId));
    }
    return *d->games[gameId-1];
}

Game &Games::byIdentityKey(char const *identityKey) const
{
    if(identityKey && identityKey[0])
    {
        foreach(Game *game, d->games)
        {
            if(!Str_CompareIgnoreCase(game->identityKey(), identityKey))
                return *game;
        }
    }
    /// @throw NotFoundError  The specified @a identityKey string is not associated with a game in the collection.
    throw NotFoundError("Games::byIdentityKey",
                        QString("There is no Game with identity key \"%1\"").arg(identityKey));
}

Game &Games::byIndex(int idx) const
{
    if(idx < 0 || idx > d->games.count())
    {
        /// @throw NotFoundError  No game is associated with index @a idx.
        throw NotFoundError("Games::byIndex", QString("There is no Game at index %i").arg(idx));
    }
    return *d->games[idx];
}

void Games::clear()
{
    d->clear();
}

Games::All const &Games::all() const
{
    return d->games;
}

int Games::collectAll(GameList &collected)
{
    int numFoundSoFar = collected.count();
    foreach(Game *game, d->games)
    {
        collected.push_back(GameListItem(game));
    }
    return collected.count() - numFoundSoFar;
}

void Games::add(Game &game)
{
    // Already a member of the collection?
    if(d->games.indexOf(&game) >= 0) return;

    d->games.push_back(&game);

    DENG2_FOR_AUDIENCE(Addition, i)
    {
        i->gameAdded(game);
    }
}

void Games::locateStartupResources(Game &game)
{
    Game *oldCurrentGame = &App_CurrentGame();
    if(oldCurrentGame != &game)
    {
        /// @attention Kludge: Temporarily switch Game.
        App_SetCurrentGame(game);
        DD_ExchangeGamePluginEntryPoints(game.pluginId());

        // Re-init the filesystem subspace schemes using the search paths of this Game.
        App_FileSystem().resetAllSchemes();
    }

    foreach(ResourceManifest *manifest, game.manifests())
    {
        // We are only interested in startup resources at this time.
        if(!(manifest->fileFlags() & FF_STARTUP)) continue;

        manifest->locateFile();
    }

    if(oldCurrentGame != &game)
    {
        // Kludge end - Restore the old Game.
        App_SetCurrentGame(*oldCurrentGame);
        DD_ExchangeGamePluginEntryPoints(oldCurrentGame->pluginId());

        // Re-init the filesystem subspace schemes using the search paths of this Game.
        App_FileSystem().resetAllSchemes();
    }
}

static int locateAllResourcesWorker(void *parameters)
{
    Games *games = (Games *) parameters;
    int n = 0;
    foreach(Game *game, games->all())
    {
        Con_Message("Locating \"%s\"...", Str_Text(game->title()));

        games->locateStartupResources(*game);
        Con_SetProgress((n + 1) * 200 / games->count() - 1);

        VERBOSE( Game::print(*game, PGF_LIST_STARTUP_RESOURCES|PGF_STATUS) )
        ++n;
    }
    BusyMode_WorkerEnd();
    return 0;
}

void Games::locateAllResources()
{
    BusyMode_RunNewTaskWithName(BUSYF_STARTUP | BUSYF_PROGRESS_BAR | (verbose? BUSYF_CONSOLE_OUTPUT : 0),
                                locateAllResourcesWorker, (void *)this, "Locating game resources...");
}

} // namespace de

D_CMD(ListGames)
{
    DENG_UNUSED(src); DENG_UNUSED(argc); DENG_UNUSED(argv);

    de::Games &games = App_Games();
    if(!games.count())
    {
        Con_Printf("No Registered Games.\n");
        return true;
    }

    //Con_FPrintf(CPF_YELLOW, "Registered Games:\n");
    //Con_Printf("Key: '!'= Incomplete/Not playable '*'= Loaded\n");

    LOG_MSG(_E(1) "Registered Games:");
    LOG_VERBOSE("Key: %s'!'=Incomplete/Not playable %s'*'=Loaded")
            << _E(>) _E(D) << _E(B);

    Con_PrintRuler();

    de::Games::GameList found;
    games.collectAll(found);
    // Sort so we get a nice alphabetical list.
    qSort(found.begin(), found.end());

    de::String list;

    int numCompleteGames = 0;
    DENG2_FOR_EACH_CONST(de::Games::GameList, i, found)
    {
        de::Game *game = i->game;
        bool isCurrent = (&App_CurrentGame() == game);

        if(!list.isEmpty()) list += "\n";

        list += de::String(_E(0)
                           _E(Ta) "%1%2 "
                           _E(Tb) "%3 "
                           _E(Tc) _E(2) "%4 " _E(i) "(%5)")
                .arg(isCurrent? _E(B) _E(b) :
                     !game->allStartupFilesFound()? _E(D) : "")
                .arg(isCurrent? "*" : !game->allStartupFilesFound()? "!" : " ")
                .arg(Str_Text(game->identityKey()))
                .arg(Str_Text(game->title()))
                .arg(Str_Text(game->author()));

        if(game->allStartupFilesFound())
            numCompleteGames++;
    }
    LOG_MSG("%s") << list;

    Con_PrintRuler();
    Con_Printf("%i of %i games playable.\n", numCompleteGames, games.count());
    Con_Printf("Use the 'load' command to load a game. For example: \"load gamename\".\n");

    return true;
}
