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

struct SinglePlayerClient {
    int playerId = 0;
    std::shared_ptr<Game> game;
    Queue<Message>& msgQueue;
    std::string playerName;

    SinglePlayerClient(int pid, std::shared_ptr<Game> g, Queue<Message>& mq,
                       const std::string& name = "{default}")
        : playerId(pid), game(std::move(g)), msgQueue(mq), playerName(name) {}

    void newPlayer();
    void sendMessage(const Message& msg);
    void update();
};
