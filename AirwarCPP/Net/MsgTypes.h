#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "json.hpp"
#include "../Core/Constants.h"

// ── Client → Server ─────────────────────────────────────────────

struct ConnectMsg {
    std::string playerName;
    nlohmann::json to_json() const { return {{"playerName", playerName}}; }
    static ConnectMsg from_json(const nlohmann::json& j) {
        return {j.value("playerName", "{default}")};
    }
};

struct DisconnectMsg {
    nlohmann::json to_json() const { return nlohmann::json::object(); }
    static DisconnectMsg from_json(const nlohmann::json&) { return {}; }
};

struct GetMsg {
    nlohmann::json to_json() const { return nlohmann::json::object(); }
    static GetMsg from_json(const nlohmann::json&) { return {}; }
};

struct KeyMsg {
    int key = 0;
    nlohmann::json to_json() const { return {{"key", key}}; }
    static KeyMsg from_json(const nlohmann::json& j) { return {j["key"].get<int>()}; }
};

struct JoyAxisMsg {
    int axis = 0;
    float value = 0;
    nlohmann::json to_json() const { return {{"axis", axis}, {"value", value}}; }
    static JoyAxisMsg from_json(const nlohmann::json& j) {
        return {j["axis"].get<int>(), j["value"].get<float>()};
    }
};

struct JoyHatMsg {
    std::vector<int> value;  // [x, y]
    nlohmann::json to_json() const { return {{"value", value}}; }
    static JoyHatMsg from_json(const nlohmann::json& j) {
        return {j["value"].get<std::vector<int>>()};
    }
};

// ── Server → Client ─────────────────────────────────────────────

struct ConnectResponseMsg {
    int player_id = -1;
    nlohmann::json to_json() const { return {{"player_id", player_id}}; }
    static ConnectResponseMsg from_json(const nlohmann::json& j) {
        return {j["player_id"].get<int>()};
    }
};

struct EntityState {
    int id = 0;
    float x = 0, y = 0, rotation = 0;
    std::string image;
    float health = 100;
    bool isReady = false;
    int player_id = -1;
    std::string name;
    int magabombQuantity = 0;
    int alpha = 255;

    nlohmann::json to_json() const {
        auto j = nlohmann::json{
            {"id", id}, {"x", x}, {"y", y},
            {"rotation", rotation}, {"image", image}
        };
        if (player_id >= 0) {
            j["health"] = health;
            j["isReady"] = isReady;
            j["player_id"] = player_id;
            j["name"] = name;
            j["magabombQuantity"] = magabombQuantity;
        }
        if (alpha < 255) j["alpha"] = alpha;
        return j;
    }

    static EntityState from_json(const nlohmann::json& j) {
        EntityState e;
        e.id = j["id"].get<int>();
        e.x = j["x"].get<float>(); e.y = j["y"].get<float>();
        e.rotation = j["rotation"].get<float>();
        e.image = j["image"].get<std::string>();
        if (j.contains("health")) e.health = j["health"].get<float>();
        if (j.contains("isReady")) e.isReady = j["isReady"].get<bool>();
        if (j.contains("player_id")) e.player_id = j["player_id"].get<int>();
        if (j.contains("name")) e.name = j["name"].get<std::string>();
        if (j.contains("magabombQuantity")) e.magabombQuantity = j["magabombQuantity"].get<int>();
        if (j.contains("alpha")) e.alpha = j["alpha"].get<int>();
        return e;
    }
};

struct ScreenInfoMsg {
    std::vector<EntityState> objects;
    bool isPaused = false;
    std::string pausePlayerName;

    nlohmann::json to_json() const {
        nlohmann::json arr = nlohmann::json::array();
        for (auto& o : objects) arr.push_back(o.to_json());
        return {{"objects", arr}, {"isPaused", isPaused},
                {"pausePlayerName", isPaused ? pausePlayerName : ""}};
    }

    static ScreenInfoMsg from_json(const nlohmann::json& j) {
        ScreenInfoMsg m;
        m.isPaused = j.value("isPaused", false);
        m.pausePlayerName = j.value("pausePlayerName", "");
        if (j.contains("objects")) {
            for (auto& o : j["objects"])
                m.objects.push_back(EntityState::from_json(o));
        }
        return m;
    }
};

struct GameStateChangedMsg {
    GameState state = GameState::mainMenu;
    nlohmann::json to_json() const { return {{"state", (int)state}}; }
    static GameStateChangedMsg from_json(const nlohmann::json& j) {
        return {static_cast<GameState>(j["state"].get<int>())};
    }
};

struct PlaySoundMsg {
    std::string sound;
    nlohmann::json to_json() const { return {{"sound", sound}}; }
    static PlaySoundMsg from_json(const nlohmann::json& j) {
        return {j["sound"].get<std::string>()};
    }
};

struct ParticleEffectMsg {
    std::string effect;
    float x = 0, y = 0;
    nlohmann::json to_json() const {
        return {{"effect", effect}, {"x", x}, {"y", y}};
    }
    static ParticleEffectMsg from_json(const nlohmann::json& j) {
        return {j["effect"].get<std::string>(),
                j["x"].get<float>(), j["y"].get<float>()};
    }
};

struct LoadLevelMsg {
    std::string level;
    nlohmann::json to_json() const { return {{"level", level}}; }
    static LoadLevelMsg from_json(const nlohmann::json& j) {
        return {j["level"].get<std::string>()};
    }
};

struct SetTitleMsg {
    std::string title;
    int duration = 0;
    nlohmann::json to_json() const {
        return {{"title", title}, {"duration", duration}};
    }
    static SetTitleMsg from_json(const nlohmann::json& j) {
        return {j["title"].get<std::string>(), j["duration"].get<int>()};
    }
};
