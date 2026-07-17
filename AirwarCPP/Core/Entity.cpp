#include "Entity.h"
#include "Constants.h"
#include "RNG.h"
#include <random>

// ---- Unit ----
Unit::Unit() {
    boundingBox = std::make_shared<BoundingBox>(100, 50);
    weapon = std::make_shared<WeaponGroup>();
}

void Unit::update() {
    Entity::update();
    if (health <= 0) isAlive = false;
    if (weapon) weapon->update();
}

// ---- Player ----
Player::Player(int player_id_) : player_id(player_id_) {
    name = std::to_string(player_id_);
    race = Race::player;
    joystickAxisList.assign(2, 0.0);
}

// ---- Enemy ----
std::mt19937 Enemy::rng{42};

Enemy::Enemy() {
    race = Race::enemy;
    weapon = std::make_shared<WeaponGroup>();
}

void Enemy::update() {
    Unit::update();
    rotation = ~velocity;
    if (weapon) weapon->setShooting(true);
    if (targetPos.has_value()) {
        faceToTargetPos();
    } else {
        randomTargetPos();
    }
    double dist = distanceToTargetPos();
    if (dist >= 0 && dist < 30) randomTargetPos();
    if (velocity == Vector{0, 0}) randomTargetPos();
}

void Enemy::randomTargetPos() {
    std::uniform_int_distribution<int> dx(0, SCREEN_W);
    std::uniform_int_distribution<int> dy(0, SCREEN_H);
    std::uniform_int_distribution<int> da(-10, 10);
    targetPos = Point{(double)dx(rng), (double)dy(rng)};
    acceleration = Vector{0, (double)da(rng) / 10.0};
}

void Enemy::faceToTargetPos() {
    if (!targetPos) return;
    Vector dir{targetPos->x - x, targetPos->y - y};
    double len = dir.length();
    if (len == 0) return;
    double refLen = acceleration.length();
    Vector vn{dir.x * (refLen / len), dir.y * (refLen / len)};
    acceleration = vn;
}

double Enemy::distanceToTargetPos() const {
    if (!targetPos) return -1;
    return std::sqrt(std::pow(targetPos->x - x, 2) + std::pow(targetPos->y - y, 2));
}

// ---- Weapon ----
Weapon::Weapon(std::shared_ptr<Projectile> bp, double fr, Race sr)
    : bulletPrototype(std::move(bp)), fireRate(fr), shooterRace(sr) {
    std::uniform_real_distribution<double> dist(0, 1);
    cooldown = dist(globalRNG()) * fr;
}

std::vector<std::shared_ptr<Projectile>> Weapon::shoot(double x, double y, int times) {
    if (!isShooting || cooldown > 0) return {};
    std::vector<std::shared_ptr<Projectile>> ret;
    for (int i = 0; i < times; ++i) {
        auto b = std::make_shared<Bullet>(); // fallback; override in subclasses
        b->shooterRace = shooterRace;
        b->x = x;
        b->y = y;
        ret.push_back(b);
    }
    cooldown = fireRate;
    return ret;
}

// ---- Projectile ----
Projectile::Projectile(double dmg, double life)
    : damage(dmg), lifetime(life) {
    boundingBox = std::make_shared<BoundingBox>(3, 8);
    velocityMultiplier = 1;
}

void Projectile::update() {
    Entity::update();
    --lifetime;
    if (lifetime <= 0) isAlive = false;
    rotation = ~velocity;
}

void Projectile::onCollision(Entity& other) {
    if (other.race != shooterRace && other.race != Race::neutral) {
        if (auto* u = dynamic_cast<Unit*>(&other)) {
            u->health -= damage;
            isAlive = false;
        }
    }
}

// ---- Concrete Projectiles ----
Bullet::Bullet() : Projectile(5, 150) {
    image = Images::bullet1;
    boundingBox = std::make_shared<BoundingBox>(3, 8);
    velocity = Vector(0, -10);
}

Lazer::Lazer() : Projectile(1.5, 150) {
    image = Images::lazer1;
    boundingBox = std::make_shared<BoundingBox>(3, 8);
    velocity = Vector(0, -15);
}

AutocannonShells::AutocannonShells() : Projectile(10, 150) {
    image = Images::autocannon12;
    boundingBox = std::make_shared<BoundingBox>(3, 7);
    velocity = Vector(0, -20);
}

Missile::Missile(int handedness_) : Projectile(4, 150), handedness(handedness_) {
    image = Images::missile;
    boundingBox = std::make_shared<BoundingBox>(12, 16);
    velocity = Vector(1 * handedness, -4);
    acceleration = Vector(0, -0.5);
}

void Missile::update() {
    Projectile::update();
    if (target && target->isAlive) {
        faceToTarget(*target, true);
        if (awaitedDelay == 0) {
            velocity *= 0.6;
            faceToTarget(*target, false);
            awaitedDelay = lockTargetDelay;
        } else {
            --awaitedDelay;
        }
    }
}

Rocket::Rocket(int handedness_) : Projectile(12, 150), handedness(handedness_) {
    image = Images::rocket;
    boundingBox = std::make_shared<BoundingBox>(8, 16);
    velocity = Vector(1 * handedness, -4);
    acceleration = Vector(0, -0.7);
}

EnergyBall::EnergyBall() : Projectile(12, 150) {
    image = Images::energyball;
    chooseTarget = true;
    boundingBox = std::make_shared<BoundingBox>(8, 8);
    velocity = Vector(0, 4);
}

EnergyBallEnhanced::EnergyBallEnhanced() : Projectile(24, 150) {
    image = Images::energyball_enhanced;
    chooseTarget = true;
    boundingBox = std::make_shared<BoundingBox>(8, 8);
    velocity = Vector(0, 6);
}

RocketEnemy::RocketEnemy() : Projectile(12, 150) {
    image = Images::rocket_enemy;
    boundingBox = std::make_shared<BoundingBox>(8, 16);
    velocity = Vector(0, 2);
    acceleration = Vector(0, 0.7);
    chooseTarget = true;
}

Magabomb::Magabomb() : Projectile(10, 2147483647) {
    image = Images::magabomb;
    boundingBox = nullptr;
    velocity = Vector(0, 10);
}

void Magabomb::explode() {
    isAlive = false;
    isExploding = true;
}

void Magabomb::update() {
    Entity::update();
    double dist = std::sqrt(std::pow(explodeX - x, 2) + std::pow(explodeY - y, 2));
    if (dist < 30) {
        explode();
    } else {
        Entity dummy(explodeX, explodeY);
        faceToTarget(dummy, false);
    }
}

// ---- WeaponGroup ----
bool WeaponGroup::has(const std::type_info& type) const {
    for (auto& w : weapons)
        if (typeid(*w) == type) return true;
    return false;
}

void WeaponGroup::removeAll(const std::type_info& type) {
    weapons.erase(std::remove_if(weapons.begin(), weapons.end(),
        [&](auto& w) { return typeid(*w) == type; }), weapons.end());
}

void WeaponGroup::upgrade(const std::type_info& type) {
    for (auto& w : weapons)
        if (typeid(*w) == type) w->upgrade();
}

int WeaponGroup::getHighestLevel() const {
    int highest = 0;
    for (auto& w : weapons)
        if (w->level > highest) highest = w->level;
    return highest;
}

void WeaponGroup::setShooting(bool v) {
    isShooting_ = v;
    for (auto& w : weapons) w->isShooting = v;
}

std::vector<std::shared_ptr<Projectile>> WeaponGroup::shoot(double x, double y) {
    std::vector<std::shared_ptr<Projectile>> ret;
    for (auto& w : weapons) {
        auto projs = w->shoot(x, y);
        ret.insert(ret.end(), projs.begin(), projs.end());
    }
    return ret;
}

void WeaponGroup::update() {
    for (auto& w : weapons) w->update();
}

// ---- Concrete Weapons ----
const std::unordered_map<int, std::vector<std::variant<int, std::pair<int,int>>>> Shotgun::bulletSpreadMap = {
    {1, {0}}, {2, {-2, 2}}, {3, {-3, 0, 3}},
    {4, {std::pair(-1,0), -2, 2, std::pair(1,0)}},
    {5, {std::pair(-2,0), std::pair(-1,0), 0, std::pair(1,0), std::pair(2,0)}},
    {6, {std::pair(-2,0), std::pair(-1,0), -3, 0, 3, std::pair(1,0), std::pair(2,0)}},
    {7, {std::pair(-3,0), std::pair(-2,0), std::pair(-1,2), std::pair(0,2), std::pair(1,2), std::pair(2,0), std::pair(3,0)}},
    {8, {std::pair(-3,0), std::pair(-2,0), std::pair(-1,2), -2, std::pair(0,2), 2, std::pair(1,2), std::pair(2,0), std::pair(3,0)}},
    {9, {std::pair(-5,0), std::pair(-3,0), std::pair(-1,2), -4, std::pair(0,2), 4, std::pair(1,2), std::pair(3,0), std::pair(5,0)}},
    {10, {std::pair(-5,0), std::pair(-3,0), std::pair(-1,2), -4, -2, std::pair(0,2), 2, 4, std::pair(1,2), std::pair(3,0), std::pair(5,0)}}
};

Shotgun::Shotgun(Race sr) : Weapon(std::make_shared<Bullet>(), 5, sr) {
    sound = Sounds::shotgun_shoot;
    jamType = WeaponJamType::shotgun;
}

std::vector<std::shared_ptr<Projectile>> Shotgun::shoot(double x, double y, int times) {
    auto it = bulletSpreadMap.find(level);
    int count = (it != bulletSpreadMap.end()) ? (int)it->second.size() : 1;
    auto projs = Weapon::shoot(x, y, count);
    if (!projs.empty()) {
        for (size_t i = 0; i < projs.size(); ++i) {
            auto& spreadList = it->second;
            auto& item = spreadList[i];
            if (std::holds_alternative<std::pair<int,int>>(item)) {
                auto p = std::get<std::pair<int,int>>(item);
                projs[i]->velocity += Vector(p.first, p.second);
            } else {
                projs[i]->x += std::get<int>(item);
            }
            if (shooterRace == Race::enemy) {
                projs[i]->chooseTarget = true;
                projs[i]->image = Images::bullet_enemy;
            }
        }
    }
    return projs;
}

LazerGun::LazerGun(Race sr) : Weapon(std::make_shared<Lazer>(), 1.5, sr) {
    sound = Sounds::lazer_shoot;
    jamType = WeaponJamType::lazer;
}

std::vector<std::shared_ptr<Projectile>> LazerGun::shoot(double x, double y, int times) {
    auto projs = Weapon::shoot(x, y, 1);
    if (!projs.empty()) {
        auto& p = projs[0];
        static const std::unordered_map<int, Images> imgMap = {
            {1, Images::lazer1}, {2, Images::lazer2}, {3, Images::lazer3},
            {4, Images::lazer4}, {5, Images::lazer5}, {6, Images::lazer6},
            {7, Images::lazer7}, {8, Images::lazer8}, {9, Images::lazer9}, {10, Images::lazer10}
        };
        static const std::unordered_map<int, double> dmgMap = {
            {1,2},{2,4},{3,6.5},{4,9},{5,11},{6,13},{7,15},{8,18},{9,20},{10,22}
        };
        auto it_i = imgMap.find(level);
        auto it_d = dmgMap.find(level);
        if (it_i != imgMap.end()) p->image = it_i->second;
        if (it_d != dmgMap.end()) {
            p->damage = it_d->second;
            p->boundingBox = std::make_shared<BoundingBox>(3 * (p->damage / 1.5), 15);
        }
        if (shooterRace == Race::enemy) p->velocity.y = -p->velocity.y;
    }
    return projs;
}

Autocannon::Autocannon(Race sr) : Weapon(std::make_shared<AutocannonShells>(), 10, sr) {
    sound = Sounds::autocannon_shoot;
    jamType = WeaponJamType::shotgun;
}

std::vector<std::shared_ptr<Projectile>> Autocannon::shoot(double x, double y, int times) {
    auto projs = Weapon::shoot(x, y, 1);
    if (!projs.empty()) {
        auto& p = projs[0];
        static const std::unordered_map<int, Images> imgMap = {
            {1, Images::autocannon12}, {2, Images::autocannon12},
            {3, Images::autocannon34}, {4, Images::autocannon34},
            {5, Images::autocannon56}, {6, Images::autocannon56},
            {7, Images::autocannon7}, {8, Images::autocannon8},
            {9, Images::autocannon9}, {10, Images::autocannon10}
        };
        static const std::unordered_map<int, double> dmgMap = {
            {1,10},{2,12},{3,15},{4,20},{5,24},{6,28},{7,31},{8,35},{9,37},{10,40}
        };
        static const std::unordered_map<int, double> velMap = {
            {1,-20},{2,-24},{3,-24},{4,-30},{5,-30},{6,-40},{7,-40},{8,-40},{9,-40},{10,-40}
        };
        auto it_i = imgMap.find(level);
        auto it_d = dmgMap.find(level);
        auto it_v = velMap.find(level);
        if (it_i != imgMap.end()) p->image = it_i->second;
        if (it_d != dmgMap.end()) p->damage = it_d->second;
        if (it_v != velMap.end()) p->velocity.y = it_v->second;
        p->chooseTargetAngle = 120;
    }
    return projs;
}

MissileLauncher::MissileLauncher(Race sr) : Weapon(std::make_shared<Missile>(), 15, sr) {
    level = 0;
    maxLevel = 0;
    sound = Sounds::missile_shoot;
}

std::vector<std::shared_ptr<Projectile>> MissileLauncher::shoot(double x, double y, int times) {
    if (!isShooting || cooldown > 0) return {};
    auto mL = std::make_shared<Missile>(1);
    auto mR = std::make_shared<Missile>(-1);
    mL->shooterRace = shooterRace; mL->x = x; mL->y = y;
    mR->shooterRace = shooterRace; mR->x = x; mR->y = y;
    cooldown = fireRate;
    return {mL, mR};
}

RocketLauncher::RocketLauncher(Race sr) : Weapon(std::make_shared<Rocket>(), 20, sr) {
    sound = Sounds::rocket_shoot;
}

std::vector<std::shared_ptr<Projectile>> RocketLauncher::shoot(double x, double y, int times) {
    if (!isShooting || cooldown > 0) return {};
    auto rL = std::make_shared<Rocket>(1);
    auto rR = std::make_shared<Rocket>(-1);
    rL->shooterRace = shooterRace; rL->x = x; rL->y = y;
    rR->shooterRace = shooterRace; rR->x = x; rR->y = y;
    cooldown = fireRate;
    return {rL, rR};
}

EnergyWeapon::EnergyWeapon(Race sr)
    : Weapon(std::make_shared<EnergyBall>(), 2.5 * GAME_TICK, sr) {
    level = 0; maxLevel = 0;
    sound = Sounds::unprepare;
    jamType = WeaponJamType::shotgun;
}

EnergyWeaponEnhanced::EnergyWeaponEnhanced(Race sr)
    : Weapon(std::make_shared<EnergyBallEnhanced>(), 3 * GAME_TICK, sr) {
    level = 0; maxLevel = 0;
    sound = Sounds::prepare;
    jamType = WeaponJamType::shotgun;
}

RocketLauncherEnemy::RocketLauncherEnemy(Race sr)
    : Weapon(std::make_shared<RocketEnemy>(), 3 * GAME_TICK, sr) {
    level = 0; maxLevel = 0;
    sound = Sounds::rocket_shoot;
    jamType = WeaponJamType::shotgun;
}

Shotgun_slow::Shotgun_slow(Race sr) : Shotgun(sr) { fireRate = 1 * GAME_TICK; }
Shotgun_normal::Shotgun_normal(Race sr) : Shotgun(sr) { fireRate = 0.5 * GAME_TICK; }
MissileLauncher_slow::MissileLauncher_slow(Race sr) : MissileLauncher(sr) {
    fireRate = 1.5 * GAME_TICK;
    jamType = WeaponJamType::shotgun;
}
