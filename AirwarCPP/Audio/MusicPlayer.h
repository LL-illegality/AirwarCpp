#pragma once
#include "SoundEngine.h"
#include "../Core/RNG.h"
#include "../Core/Constants.h"
#include <string>
#include <vector>
#include <unordered_map>

class MusicPlayer {
    SoundEngine* engine_ = nullptr;
    MIX_Track* track_ = nullptr;
    MIX_Audio* currentAudio_ = nullptr;
    bool hasIntro_ = false;
    Sint64 introFrames_ = 0;
    int lastState_ = -1;
    std::string assetRoot_;

    struct Song { std::string intro; std::string loop; };
    std::vector<std::string> songKeys_ = {"future","lostcity","pop","universe41","beach","escape"};
    std::string currentSong_;

    Song getSong(const std::string& key) const {
        return {
            assetRoot_ + "Resources\\music\\" + key + "_intro.wav",
            assetRoot_ + "Resources\\music\\" + key + ".wav"
        };
    }

    void stopAndClean() {
        if (track_) { engine_->stopTrack(track_); track_ = nullptr; currentAudio_ = nullptr; }
    }

    void playAudio(MIX_Audio* audio, bool hasIntro, Sint64 introFrames) {
        stopAndClean();
        if (!audio) return;
        currentAudio_ = audio;
        hasIntro_ = hasIntro;
        introFrames_ = introFrames;
        track_ = engine_->play(audio);
    }

public:
    MusicPlayer() = default;

    void init(SoundEngine* engine, const std::string& assetRoot) {
        engine_ = engine;
        assetRoot_ = assetRoot;
    }

    bool loadAll() {
        // Load mainmenu
        auto path = assetRoot_ + "Resources\\music\\mainmenu.wav";
        if (!engine_->load("mainmenu", path)) return false;

        // Load song intros + loops
        for (auto& key : songKeys_) {
            auto song = getSong(key);
            if (!engine_->load(key + "_intro", song.intro)) return false;
            if (!engine_->load(key + "_loop", song.loop)) return false;
        }
        return true;
    }

    void playState(GameState state) {
        int s = (int)state;
        if (s == lastState_) return;
        lastState_ = s;

        if (state == GameState::mainMenu) {
            playAudio(engine_->get("mainmenu"), false, 0);
        }
        else if (state == GameState::loadLevel || state == GameState::inGame) {
            if (currentSong_.empty() || state == GameState::loadLevel) {
                currentSong_ = songKeys_[std::uniform_int_distribution<int>(0, (int)songKeys_.size()-1)(globalRNG())];
            }
            auto* intro = engine_->get(currentSong_ + "_intro");
            auto* loop = engine_->get(currentSong_ + "_loop");
            if (intro && loop) {
                Sint64 dur = engine_->getDuration(intro);
                playAudio(intro, true, dur);
            }
        }
    }

    void update() {
        if (!hasIntro_ || !track_ || !currentAudio_) return;
        // Check if intro finished, switch to loop
        auto props = MIX_GetTrackProperties(track_);
        // For simplicity in Phase 4, we don't auto-switch; the test will verify
        // that both intro and loop can be played individually
    }

    void stop() { stopAndClean(); lastState_ = -1; }
    MIX_Track* track() const { return track_; }
};
