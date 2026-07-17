#pragma once
#include "../Core/Constants.h"
#include "../Config/ConfigPersistence.h"
#include <string>
#include <vector>
#include <functional>

class Tutorial {
    int step_ = 0;
    int waitTime_ = 0;
    bool active_ = false;
    bool completed_ = false;
    bool enabled_ = true;
    std::vector<std::string> messages_;

    std::function<void()> onSpawnEnemy_;
    std::function<void()> onTutorialComplete_;

public:
    void setCallbacks(std::function<void()> onSpawn, std::function<void()> onComplete) {
        onSpawnEnemy_ = std::move(onSpawn);
        onTutorialComplete_ = std::move(onComplete);
    }

    void loadConfig(const std::string& assetRoot) {
        auto cfg = GameConfig::load(assetRoot + "Resources\\configs\\initializeSettings.json");
        enabled_ = cfg.showTutorial;
    }

    void saveConfig(const std::string& assetRoot) {
        auto cfg = GameConfig::load(assetRoot + "Resources\\configs\\initializeSettings.json");
        cfg.showTutorial = false;
        GameConfig::save(assetRoot + "Resources\\configs\\initializeSettings.json", cfg);
    }

    bool shouldShow() const { return enabled_; }
    bool isActive() const { return active_; }
    bool isComplete() const { return completed_; }
    const std::vector<std::string>& messages() const { return messages_; }

    void start() {
        if (!enabled_) { completed_ = true; return; }
        active_ = true;
        step_ = 0;
        waitTime_ = 0;
        messages_.clear();
    }

    void update() {
        if (!active_ || completed_) return;
        if (waitTime_ == 0) advanceStep();
        else --waitTime_;
    }

    void finish() {
        active_ = false;
        completed_ = true;
        messages_.clear();
        if (onTutorialComplete_) onTutorialComplete_();
    }

private:
    void addMessage(const std::string& msg, int duration) {
        messages_.push_back(msg);
        waitTime_ = duration;
    }

    void advanceStep() {
        ++step_;
        messages_.clear();

        switch (step_) {
            case 1: addMessage("--- Airwar ---", 5 * GAME_TICK); break;
            case 2: addMessage("Welcome to Airwar Tutorial", 3 * GAME_TICK); break;
            case 3: addMessage("Use WASD to move", 10 * GAME_TICK); break;
            case 4: addMessage("Press SPACE to shoot", 3 * GAME_TICK); break;
            case 5: addMessage("Press Z to draw marker", 3 * GAME_TICK); break;
            case 6: addMessage("Marker helps locate yourself in multiplayer", 3 * GAME_TICK); break;
            case 7: addMessage("Hold C to prepare", 3 * GAME_TICK); break;
            case 8: addMessage("Game starts when everyone is ready", 3 * GAME_TICK); break;
            case 9:
                addMessage("Enemy appears! Move and shoot!", 2147483647);
                if (onSpawnEnemy_) onSpawnEnemy_();
                break;
            case 10: addMessage("Good!", 2 * GAME_TICK); break;
            case 11: addMessage("Enemies may drop items", 2 * GAME_TICK); break;
            case 12:
                addMessage("Try again", 2147483647);
                if (onSpawnEnemy_) onSpawnEnemy_();
                break;
            case 13: addMessage("Different items have different effects", 2 * GAME_TICK); break;
            case 14: addMessage("Upgrade weapons, heal, or swap weapons", 2 * GAME_TICK); break;
            case 15:
                addMessage("Collect items!", 2147483647);
                if (onSpawnEnemy_) onSpawnEnemy_();
                break;
            case 16: addMessage("If overwhelmed by enemies", 2 * GAME_TICK); break;
            case 17: addMessage("Check the bottom-right for nuke icon", 2 * GAME_TICK); break;
            case 18: addMessage("Press E to use nuke", 2 * GAME_TICK); break;
            case 19: addMessage("Nuke eliminates all enemies", 2 * GAME_TICK); break;
            case 20:
                addMessage("Use the nuke!", 2147483647);
                if (onSpawnEnemy_) onSpawnEnemy_();
                break;
            case 21: addMessage("You've learned everything!", 3 * GAME_TICK); break;
            case 22: addMessage("Tutorial complete", 3 * GAME_TICK); break;
            case 23: addMessage("Press C to ready up!", 2 * GAME_TICK); break;
            case 24: finish(); break;
            default: finish(); break;
        }
    }
};
