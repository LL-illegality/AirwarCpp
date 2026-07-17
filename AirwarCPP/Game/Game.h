#pragma once
#include "../Core/Constants.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "../Core/Level.h"
#include "Board.h"
#include <string>
#include <vector>
#include <memory>

struct Game {
    GameState currState_ = GameState::mainMenu;
    GameState currState() const { return currState_; }
    void setCurrState(GameState s);

    Queue<Message>& msgQueue;
    Board board;
    bool isPaused = false;
    int pausePlayerId = -1;
    std::string pausePlayerName;

    struct PendingEnemy {
        Enemy enemy;
        int delay;
    };
    std::vector<PendingEnemy> pendingEnemies;

    explicit Game(Queue<Message>& mq);

    void getObjects();
    void setWaitTime(int t) { waitTime = t; }
    bool isWaitTimeOver();
    void update();
    void detectLevelState();
    void addFlagUnit(const Flag& flag);
    Flag maybeSplitFlag(const Level& level, const Flag& flag);
    void processPendingEnemies();

private:
    int waitTime = 0;
};
