#pragma once
#include "../Core/Entity.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

inline std::shared_ptr<Weapon> createWeapon(const std::string& name, Race shooterRace) {
    static const std::unordered_map<std::string, std::function<std::shared_ptr<Weapon>(Race)>> factory = {
        {"Shotgun", [](Race r) { return std::make_shared<Shotgun>(r); }},
        {"Shotgun_slow", [](Race r) { return std::make_shared<Shotgun_slow>(r); }},
        {"Shotgun_normal", [](Race r) { return std::make_shared<Shotgun_normal>(r); }},
        {"LazerGun", [](Race r) { return std::make_shared<LazerGun>(r); }},
        {"Autocannon", [](Race r) { return std::make_shared<Autocannon>(r); }},
        {"MissileLauncher", [](Race r) { return std::make_shared<MissileLauncher>(r); }},
        {"MissileLauncher_slow", [](Race r) { return std::make_shared<MissileLauncher_slow>(r); }},
        {"RocketLauncher", [](Race r) { return std::make_shared<RocketLauncher>(r); }},
        {"RocketLauncherEnemy", [](Race r) { return std::make_shared<RocketLauncherEnemy>(r); }},
        {"EnergyWeapon", [](Race r) { return std::make_shared<EnergyWeapon>(r); }},
        {"EnergyWeaponEnhanced", [](Race r) { return std::make_shared<EnergyWeaponEnhanced>(r); }}
    };
    auto it = factory.find(name);
    if (it != factory.end()) return it->second(shooterRace);
    return nullptr;
}

inline std::shared_ptr<WeaponGroup> createWeaponGroup(const std::vector<std::string>& names, Race shooterRace) {
    auto group = std::make_shared<WeaponGroup>();
    for (auto& n : names) {
        auto w = createWeapon(n, shooterRace);
        if (w) group->addWeapon(w);
    }
    return group;
}
