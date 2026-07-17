#pragma once
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

class SoundEngine {
    MIX_Mixer* mixer_ = nullptr;
    std::unordered_map<std::string, MIX_Audio*> sounds_;
    std::vector<MIX_Track*> activeTracks_;

public:
    ~SoundEngine() { cleanup(); }

    bool init() {
        if (mixer_) return true;
        if (!MIX_Init()) { SDL_Log("MIX_Init failed: %s", SDL_GetError()); return false; }
        mixer_ = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (!mixer_) { SDL_Log("MIX_CreateMixerDevice failed: %s", SDL_GetError()); return false; }
        return true;
    }

    void cleanup() {
        if (!mixer_) return;
        activeTracks_.clear();
        sounds_.clear();
        mixer_ = nullptr;
    }

    MIX_Mixer* mixer() const { return mixer_; }

    static void shutdown() {
        // Call once at process exit to release all mixer/track/audio resources
        MIX_Quit();
    }

    MIX_Audio* load(const std::string& name, const std::string& path, bool predecode = true) {
        auto it = sounds_.find(name);
        if (it != sounds_.end()) return it->second;
        auto* audio = MIX_LoadAudio(mixer_, path.c_str(), predecode);
        if (!audio) { SDL_Log("MIX_LoadAudio(%s) failed: %s", path.c_str(), SDL_GetError()); return nullptr; }
        sounds_[name] = audio;
        return audio;
    }

    MIX_Track* play(MIX_Audio* audio) {
        if (!audio || !mixer_) return nullptr;
        auto* track = MIX_CreateTrack(mixer_);
        if (!track) return nullptr;
        MIX_SetTrackAudio(track, audio);
        if (!MIX_PlayTrack(track, 0)) { /* track will be cleaned up by MIX_Quit */ return nullptr; }
        activeTracks_.push_back(track);
        return track;
    }

    MIX_Track* play(const std::string& name) {
        auto it = sounds_.find(name);
        if (it == sounds_.end()) return nullptr;
        return play(it->second);
    }

    void stopTrack(MIX_Track* track) {
        if (!track) return;
        MIX_StopTrack(track, 0);
        // Don't destroy track manually — MIX_Quit handles cleanup
    }

    void stopAll() {
        for (auto& t : activeTracks_) if (t) MIX_StopTrack(t, 0);
        activeTracks_.clear();
    }

    MIX_Audio* get(const std::string& name) {
        auto it = sounds_.find(name);
        return it != sounds_.end() ? it->second : nullptr;
    }

    Sint64 getDuration(MIX_Audio* audio) const {
        return audio ? MIX_GetAudioDuration(audio) : 0;
    }
};
