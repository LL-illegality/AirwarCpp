#pragma once
#include "Constants.h"
#include "Entity.h"
#include "RNG.h"
#include "json.hpp"
#include <string>
#include <vector>
#include <random>
#include <filesystem>
#include <fstream>

struct Flag {
    std::vector<std::string> unitTypeList;
    int timeBeforeNext = 0;
    FlagFinishCondition finishCondition = FlagFinishCondition::killAll;
    std::vector<int> drops;
    bool isFinished = false;

    Flag() = default;
    Flag(std::vector<std::string> units, int wait, FlagFinishCondition fc)
        : unitTypeList(std::move(units)), timeBeforeNext(wait), finishCondition(fc) {}

    std::vector<std::string> getUnits() {
        isFinished = true;
        return unitTypeList;
    }

    static Flag from_json(const nlohmann::json& j) {
        Flag f;
        for (auto& u : j["unitTypeList"]) f.unitTypeList.push_back(u.get<std::string>());
        f.timeBeforeNext = j["timeBeforeNext"].get<int>();
        f.finishCondition = static_cast<FlagFinishCondition>(j["finishCondition"].get<int>());
        return f;
    }
};

struct Level {
    std::string name;
    int currFlagIndex = -1;
    int totalFlags = 0;
    static Flag splitFlagStatic(const Flag& flag, int avgLevel, int nukeBonus);
    bool waitLoaded = false;
    bool isFinished = false;
    std::vector<Flag> flags;
    std::vector<int> drops;

    Level() = default;
    Level(const std::string& name_, int total, const std::vector<Flag>& flags_,
          std::vector<int> drops_)
        : name(name_), totalFlags(total), flags(flags_), drops(std::move(drops_)) {
        for (auto& item : drops) {
            auto& flag = flags[std::uniform_int_distribution<int>(0, totalFlags - 1)(globalRNG())];
            flag.drops.push_back(item);
        }
    }

    void nextFlag() {
        ++currFlagIndex;
        if (currFlagIndex >= totalFlags) {
            isFinished = true;
            return;
        }
        flags[currFlagIndex].isFinished = false;
    }

    Flag* getCurrFlag() {
        if (currFlagIndex < 0 || currFlagIndex >= totalFlags) return nullptr;
        return &flags[currFlagIndex];
    }

    Flag* loadFlag() {
        nextFlag();
        if (isFinished) return nullptr;
        return &flags[currFlagIndex];
    }

    Flag splitFlag(const Flag& flag, int avgLevel, int nukeBonus = 0);
};

struct EnemyBuilder {
    double health = 100;
    std::string image = "en";
    std::vector<std::string> weapon;
    Vector velocity;
    Vector acceleration;
    std::shared_ptr<BoundingBox> boundingBox;
    std::optional<Point> targetPos;
    double maxVelocity = 0;
    double velocityMultiplier = 0.9;
    std::vector<int> inventory;

    Enemy build() const;
};

struct LevelLoader {
    int currLevel = -1;
    std::string path;
    std::vector<std::string> levelFiles;
    int totalLevel = 0;
    bool isFinished = false;
    std::vector<Level> levelData;
    nlohmann::json enemyTypes;

    explicit LevelLoader(std::string path_ = "AirwarCPP\\Resources\\levels")
        : path(std::move(path_)) {}

    void loadLevels() {
        for (auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.path().extension() == ".json")
                levelFiles.push_back(entry.path().string());
        }
        std::sort(levelFiles.begin(), levelFiles.end());
        totalLevel = (int)levelFiles.size();
        for (auto& fp : levelFiles) {
            std::ifstream f(fp);
            nlohmann::json j;
            f >> j;
            std::vector<Flag> flags;
            for (auto& fj : j["flags"])
                flags.push_back(Flag::from_json(fj));
            std::vector<int> drops;
            if (j.contains("drops")) {
                for (auto& d : j["drops"])
                    drops.push_back(d.get<int>());
            }
            levelData.emplace_back(
                j["name"].get<std::string>(),
                j["totalFlags"].get<int>(),
                flags, drops);
        }
    }

    void createAttr() {
        for (auto& [key, val] : enemyTypes.items()) {
            auto& v = val;
            if (v.contains("velocity")) {
                v["velocity"] = nlohmann::json{{"x", v["velocity"]["x"]}, {"y", v["velocity"]["y"]}};
            }
        }
    }

    std::vector<Enemy> createUnits(const std::vector<std::string>& unitTypeList) {
        std::vector<Enemy> units;
        for (auto& type : unitTypeList) {
            auto it = enemyTypes.find(type);
            if (it == enemyTypes.end()) continue;
            EnemyBuilder builder;
            auto& j = it.value();
            if (j.contains("health")) builder.health = j["health"].get<double>();
            if (j.contains("image")) builder.image = j["image"].get<std::string>();
            if (j.contains("velocity")) builder.velocity = {j["velocity"]["x"].get<double>(), j["velocity"]["y"].get<double>()};
            if (j.contains("acceleration")) builder.acceleration = {j["acceleration"]["x"].get<double>(), j["acceleration"]["y"].get<double>()};
            if (j.contains("boundingBox")) builder.boundingBox = std::make_shared<BoundingBox>(j["boundingBox"]["width"].get<double>(), j["boundingBox"]["height"].get<double>());
            if (j.contains("targetPos")) builder.targetPos = Point{j["targetPos"][0].get<double>(), j["targetPos"][1].get<double>()};
            if (j.contains("maxVelocity")) builder.maxVelocity = j["maxVelocity"].get<double>();
            if (j.contains("velocityMultiplier")) builder.velocityMultiplier = j["velocityMultiplier"].get<double>();
            if (j.contains("inventory")) { for (auto& i : j["inventory"]) builder.inventory.push_back(i.get<int>()); }
            if (j.contains("weapon")) { for (auto& w : j["weapon"]) builder.weapon.push_back(w.get<std::string>()); }
            auto enemy = builder.build();
            enemy.race = Race::enemy;
            units.push_back(std::move(enemy));
        }
        return units;
    }

    void nextLevel() {
        ++currLevel;
        if (currLevel >= totalLevel) { isFinished = true; currLevel = 147; }
    }

    Level* getCurrLevel() {
        if (currLevel < 0 || currLevel >= totalLevel || isFinished) return nullptr;
        return &levelData[currLevel];
    }

    std::optional<bool> getCurrLevelFinishState() {
        if (isFinished || currLevel < 0 || currLevel >= totalLevel) return std::nullopt;
        return levelData[currLevel].isFinished;
    }

    Level* loadLevel() {
        nextLevel();
        if (isFinished || currLevel == 147) return nullptr;
        return &levelData[currLevel];
    }
};
