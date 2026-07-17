#pragma once
#include "../Core/Constants.h"
#include <string>
#include <fstream>
#include "json.hpp"

struct GameConfig {
    std::string mode = "single";
    std::string ip = "127.0.0.1";
    int port = 0;
    std::string playerName = "{default}";
    bool showTutorial = true;

    nlohmann::json toJson() const {
        return {{"mode", mode}, {"ip", ip}, {"port", port},
                {"playerName", playerName}, {"showTutorial", showTutorial}};
    }

    static GameConfig fromJson(const nlohmann::json& j) {
        GameConfig cfg;
        if (j.contains("mode")) cfg.mode = j["mode"].get<std::string>();
        if (j.contains("ip")) cfg.ip = j["ip"].get<std::string>();
        if (j.contains("port")) cfg.port = j["port"].get<int>();
        if (j.contains("playerName")) cfg.playerName = j["playerName"].get<std::string>();
        if (j.contains("showTutorial")) cfg.showTutorial = j["showTutorial"].get<bool>();
        return cfg;
    }

    static GameConfig load(const std::string& path) {
        std::ifstream f(path);
        if (!f.good()) return GameConfig{};
        nlohmann::json j; f >> j;
        return fromJson(j);
    }

    static bool save(const std::string& path, const GameConfig& cfg) {
        std::ofstream f(path);
        if (!f.good()) return false;
        f << cfg.toJson().dump(4);
        return true;
    }
};
