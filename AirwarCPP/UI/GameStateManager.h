#pragma once
#include "../Core/Constants.h"
#include <functional>

class GameStateManager {
    GameState state_ = GameState::mainMenu;
    GameState prevState_ = GameState::mainMenu;
    bool paused_ = false;
    int pausePlayerId_ = -1;
    std::string pausePlayerName_;

    std::function<void(GameState)> onStateChanged_;

public:
    void setCallback(std::function<void(GameState)> cb) { onStateChanged_ = std::move(cb); }

    GameState state() const { return state_; }
    GameState prevState() const { return prevState_; }
    bool paused() const { return paused_; }
    int pausePlayerId() const { return pausePlayerId_; }
    std::string pausePlayerName() const { return pausePlayerName_; }

    void setState(GameState s) {
        if (state_ == s) return;
        prevState_ = state_;
        state_ = s;
        if (onStateChanged_) onStateChanged_(state_);
    }

    void togglePause(int playerId, const std::string& playerName) {
        if (!paused_) {
            paused_ = true;
            pausePlayerId_ = playerId;
            pausePlayerName_ = playerName;
        } else if (pausePlayerId_ == playerId) {
            paused_ = false;
            pausePlayerId_ = -1;
            pausePlayerName_.clear();
        }
    }

    void resume() {
        paused_ = false;
        pausePlayerId_ = -1;
        pausePlayerName_.clear();
    }

    bool canStart() const { return state_ == GameState::mainMenu; }
    bool isInGame() const { return state_ == GameState::inGame; }
    bool isGameOver() const { return state_ == GameState::gameOver; }
    bool isGameWin() const { return state_ == GameState::gameWin; }
    bool isLoading() const { return state_ == GameState::loadLevel; }

    void startGame() { setState(GameState::loadLevel); }
    void finishLoading() { setState(GameState::inGame); }
    void loseGame() { setState(GameState::gameOver); }
    void winGame() { setState(GameState::gameWin); }
    void backToMenu() { setState(GameState::mainMenu); paused_ = false; }
};
