#include "Game.h"
#include "../Core/Level.h"
#include "../Core/Constants.h"
#include "../Core/RNG.h"
#include "WeaponFactory.h"
#include <algorithm>

static std::mt19937& gameRng() { return globalRNG(); }

Game::Game(Queue<Message>& mq)
    : msgQueue(mq), board(mq) {}

void Game::setCurrState(GameState s) {
    currState_ = s;
    msgQueue.push(Message("server", "game_state_changed", {{"state", (int)s}}));
}

void Game::getObjects() {
    auto objList = board.getScreenObjects();
    msgQueue.push(Message("server", "screen_info", {
        {"objects", objList},
        {"isPaused", isPaused},
        {"pausePlayerName", isPaused ? pausePlayerName : ""}
    }));
}

bool Game::isWaitTimeOver() {
    if (waitTime <= 0) return true;
    --waitTime;
    return false;
}

void Game::processPendingEnemies() {
    std::vector<PendingEnemy> remaining;
    for (auto& pe : pendingEnemies) {
        if (pe.delay <= 0) {
            auto e = std::make_shared<Enemy>(std::move(pe.enemy));
            board.addUnit(e, "unit");
        } else {
            remaining.push_back({std::move(pe.enemy), pe.delay - 1});
        }
    }
    pendingEnemies = std::move(remaining);
}

void Game::addFlagUnit(const Flag& flag) {
    setCurrState(GameState::inGame);
    setWaitTime(flag.timeBeforeNext);
    auto unitList = flag.unitTypeList;
    std::vector<int> dropsCopy = flag.drops;

    for (auto& type : unitList) {
        auto e = Enemy();  // default, will be configured from unit type
        board.addUnit(std::make_shared<Enemy>(std::move(e)), "unit");
    }

    while (!dropsCopy.empty()) {
        // Simplified: add drops to random units
        if (!board.units.empty()) {
            auto target = std::dynamic_pointer_cast<Unit>(
                board.units[std::uniform_int_distribution<int>(0, (int)board.units.size()-1)(gameRng())]);
            if (target) {
                target->inventory.push_back(static_cast<ItemTypes>(dropsCopy.back()));
                dropsCopy.pop_back();
            } else break;
        } else break;
    }
}

Flag Game::maybeSplitFlag(const Level& level, const Flag& flag) {
    if (board.players.empty()) return flag;
    int avg = 0;
    for (auto& p : board.players) avg += p->weapon->getHighestLevel();
    avg /= (int)board.players.size();
    if (avg < MIN_WEAPON_LEVEL_THRESHOLD && (int)flag.unitTypeList.size() > MIN_ENEMY_COUNT_THRESHOLD) {
        int totalNukes = 0;
        for (auto& p : board.players) totalNukes += p->magabombQuantity;
        int nukeBonus = (totalNukes <= 2) ? totalNukes : 2 * (totalNukes - 2) + 2;
        return Level::splitFlagStatic(flag, avg, nukeBonus);
    }
    return flag;
}

void Game::detectLevelState() {
    if (isPaused) return;
    if (!isWaitTimeOver()) return;

    setWaitTime(std::uniform_int_distribution<int>(2, 5)(gameRng()) * GAME_TICK);

    std::vector<std::shared_ptr<Unit>> enemyList;
    for (auto& u : board.units) {
        auto unit = std::dynamic_pointer_cast<Unit>(u);
        if (unit && unit->race == Race::enemy) enemyList.push_back(unit);
    }

    if (!enemyList.empty() || !pendingEnemies.empty()) return;

    if (currState_ == GameState::mainMenu) {
        if (!board.isAllPlayerPrepared() || board.players.empty()) return;
    }

    // Simplified level progression — Phase 2 uses a basic tick-based approach
    // Full LevelLoader integration will come in later phases
    setCurrState(GameState::inGame);
}

void Game::update() {
    board.update();
    processPendingEnemies();
    getObjects();
}

// SinglePlayerClient moved to Net/GameClient.h