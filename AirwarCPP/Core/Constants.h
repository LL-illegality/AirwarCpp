#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

inline constexpr int SCREEN_W = 800;
inline constexpr int SCREEN_H = 600;
inline constexpr int CELL_SIZE = 100;
inline constexpr int GAME_TICK = 30;
inline constexpr double PI = 3.141592653589793;

inline constexpr int MIN_WEAPON_LEVEL_THRESHOLD = 6;
inline constexpr int MIN_ENEMY_COUNT_THRESHOLD = 4;

struct Keys {
    static constexpr int a = 97;
    static constexpr int s = 115;
    static constexpr int d = 100;
    static constexpr int w = 119;
    static constexpr int c = 99;
    static constexpr int e = 101;
    static constexpr int p = 112;
    static constexpr int space = 32;
    static constexpr int esc = 27;
};

enum class ItemTypes : int {
    shotgun = 0,
    lazer = 1,
    missile = 2,
    super = 3,
    rocket = 4,
    magabomb = 5,
    medic = 6,
    autocannon = 7
};

enum class Images : int {
    player1, player2,
    bullet1, bullet_enemy,
    lazer1, lazer2, lazer3, lazer4, lazer5,
    lazer6, lazer7, lazer8, lazer9, lazer10,
    autocannon12, autocannon34, autocannon56,
    autocannon7, autocannon8, autocannon9, autocannon10,
    missile, unit1, big1, big2, rocket, rocket_enemy,
    energyball, energyball_enhanced, magabomb,
    en, enemy2, enemy3, enemy4, enemy5,
    ready, enemy, rship, rship2, rship3, rship4,
    item_shotgun, item_missile, item_lazer, item_autocannon,
    item_super, item_rocket, item_maga, item_medic, ca
};

inline const std::unordered_map<Images, std::string> ImageNames = {
    {Images::player1, "player1"}, {Images::player2, "player2"},
    {Images::bullet1, "bullet1"}, {Images::bullet_enemy, "bullet_enemy"},
    {Images::lazer1, "lazer_level1"}, {Images::lazer2, "lazer_level2"},
    {Images::lazer3, "lazer_level3"}, {Images::lazer4, "lazer_level4"},
    {Images::lazer5, "lazer_level5"}, {Images::lazer6, "lazer_level6"},
    {Images::lazer7, "lazer_level7"}, {Images::lazer8, "lazer_level8"},
    {Images::lazer9, "lazer_level9"}, {Images::lazer10, "lazer_level10"},
    {Images::autocannon12, "autocannon_level12"},
    {Images::autocannon34, "autocannon_level34"},
    {Images::autocannon56, "autocannon_level56"},
    {Images::autocannon7, "autocannon_level7"},
    {Images::autocannon8, "autocannon_level8"},
    {Images::autocannon9, "autocannon_level9"},
    {Images::autocannon10, "autocannon_level10"},
    {Images::missile, "missile"}, {Images::unit1, "unit1"},
    {Images::big1, "big1"}, {Images::big2, "big2"},
    {Images::rocket, "rocket"}, {Images::rocket_enemy, "rocket_enemy"},
    {Images::energyball, "energyball"},
    {Images::energyball_enhanced, "energyball_enhanced"},
    {Images::magabomb, "magabomb"},
    {Images::en, "en"}, {Images::enemy2, "enemy2"},
    {Images::enemy3, "enemy3"}, {Images::enemy4, "enemy4"},
    {Images::enemy5, "enemy5"}, {Images::ready, "ready"},
    {Images::enemy, "enemy"}, {Images::rship, "rship"},
    {Images::rship2, "rship2"}, {Images::rship3, "rship3"},
    {Images::rship4, "rship4"},
    {Images::item_shotgun, "item_shotgun"},
    {Images::item_missile, "item_missile"},
    {Images::item_lazer, "item_lazer"},
    {Images::item_autocannon, "item_autocannon"},
    {Images::item_super, "item_super"},
    {Images::item_rocket, "item_rocket"},
    {Images::item_maga, "item_maga"},
    {Images::item_medic, "item_medic"},
    {Images::ca, "ca"}
};

inline Images imageFromName(const std::string& name) {
    for (auto& [k, v] : ImageNames)
        if (v == name) return k;
    return Images::en;
}

inline const std::unordered_map<ItemTypes, Images> ItemImageMap = {
    {ItemTypes::shotgun, Images::item_shotgun},
    {ItemTypes::lazer, Images::item_lazer},
    {ItemTypes::missile, Images::item_missile},
    {ItemTypes::super, Images::item_super},
    {ItemTypes::rocket, Images::item_rocket},
    {ItemTypes::magabomb, Images::item_maga},
    {ItemTypes::medic, Images::item_medic},
    {ItemTypes::autocannon, Images::item_autocannon}
};

enum class Sounds : int {
    prepare, unprepare, transmission,
    explode1, explode2, explode3, explode4, explode5,
    missile_shoot, rocket_shoot, nuclear_missile_shoot,
    nuclear_missile_explode, lazer_shoot, shotgun_shoot,
    autocannon_shoot, itemget
};

enum class Race : int { player = 0, enemy = 1, neutral = 2 };
enum class GameState : int {
    mainMenu = 0, loadLevel = 1, inGame = 2,
    gameOver = 3, pause = 4, gameWin = 5
};
enum class WeaponJamType : int { none = 0, shotgun = 1, lazer = 2 };
enum class FlagFinishCondition : int { waitForTime = 0, killAll = 1 };
enum class GameMode : int { single = 0, multiHost = 1, multiJoin = 2 };
enum class SpecialEnemyBehavior : int { none = 0, boss = 1, boom = 2 };
enum class ParticleEffect : int {
    enemy_explosion, player_explosion, missile_hit, rocket_hit,
    nuke_explosion, bullet_hit, lazer_hit, autocannon_hit
};
