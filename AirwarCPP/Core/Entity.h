#pragma once
#include "Constants.h"
#include "Vector.h"
#include "Message.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <optional>
#include <variant>
#include <algorithm>
#include <random>

struct BoundingBox {
    double width = 0;
    double height = 0;

    BoundingBox() = default;
    BoundingBox(double w, double h) : width(w), height(h) {}
};

struct Point {
    double x = 0, y = 0;
    Point() = default;
    Point(double x_, double y_) : x(x_), y(y_) {}
    Vector directionTo(const Point& other) const {
        return {other.x - x, other.y - y};
    }
};

struct Entity {
    double x = 0, y = 0;
    int id = -1;
    std::optional<Images> image;
    std::shared_ptr<BoundingBox> boundingBox;
    Vector velocity;
    Vector acceleration;
    double rotation = 0;
    double maxVelocity = 0;
    double velocityMultiplier = 0.9;
    Race race = Race::neutral;
    bool isAlive = true;

    Entity() = default;
    Entity(double x_, double y_, int id_ = -1) : x(x_), y(y_), id(id_) {}
    virtual ~Entity() = default;

    virtual void update() {
        x += velocity.x;
        y += velocity.y;
        velocity *= velocityMultiplier;
        velocity += acceleration;
        if (maxVelocity > 0) {
            if (velocity.x > maxVelocity) velocity.x = maxVelocity;
            if (velocity.x < -maxVelocity) velocity.x = -maxVelocity;
            if (velocity.y > maxVelocity) velocity.y = maxVelocity;
            if (velocity.y < -maxVelocity) velocity.y = -maxVelocity;
        }
        if (velocity.length() < 0.01) velocity = Vector{0, 0};
    }

    void faceToTarget(const Entity& target, bool useAcceleration = false) {
        Vector dir = {target.x - x, target.y - y};
        double len = dir.length();
        if (len == 0) return;
        double refLen = useAcceleration ? acceleration.length() : velocity.length();
        Vector vn = {dir.x * (refLen / len), dir.y * (refLen / len)};
        if (useAcceleration) acceleration = vn;
        else velocity = vn;
    }

    virtual void onCollision(Entity& other) {}

    bool operator&(const Entity& other) const {
        if (!boundingBox || !other.boundingBox) return false;
        auto [lu1, rd1] = getRect();
        auto [lu2, rd2] = other.getRect();
        return lu1.x < rd2.x && lu2.x < rd1.x && lu1.y < rd2.y && lu2.y < rd1.y;
    }

    bool operator==(const Entity& other) const { return id == other.id; }

    struct Hash { size_t operator()(const Entity* e) const { return std::hash<int>{}(e->id); } };
    struct Eq { bool operator()(const Entity* a, const Entity* b) const { return a->id == b->id; } };

private:
    std::pair<Point, Point> getRect() const {
        if (!boundingBox) return {};
        return {{x - boundingBox->width / 2, y - boundingBox->height / 2},
                {x + boundingBox->width / 2, y + boundingBox->height / 2}};
    }
};

struct SpatialGrid {
    std::unordered_map<int, std::unordered_set<Entity*>> cells;

    void add(Entity* entity) {
        if (!entity->boundingBox) return;
        int left   = int((entity->x - entity->boundingBox->width  / 2) / CELL_SIZE);
        int right  = int((entity->x + entity->boundingBox->width  / 2) / CELL_SIZE);
        int top    = int((entity->y - entity->boundingBox->height / 2) / CELL_SIZE);
        int bottom = int((entity->y + entity->boundingBox->height / 2) / CELL_SIZE);
        for (int cx = left; cx <= right; ++cx)
            for (int cy = top; cy <= bottom; ++cy)
                cells[cx * 10000 + cy].insert(entity);  // simple spatial hash
    }

    std::vector<Entity*> getNearby(const Entity* entity) const {
        if (!entity->boundingBox) return {};
        std::unordered_set<Entity*> seen;
        std::vector<Entity*> candidates;
        int left   = int((entity->x - entity->boundingBox->width  / 2) / CELL_SIZE);
        int right  = int((entity->x + entity->boundingBox->width  / 2) / CELL_SIZE);
        int top    = int((entity->y - entity->boundingBox->height / 2) / CELL_SIZE);
        int bottom = int((entity->y + entity->boundingBox->height / 2) / CELL_SIZE);
        for (int cx = left; cx <= right; ++cx) {
            for (int cy = top; cy <= bottom; ++cy) {
                auto it = cells.find(cx * 10000 + cy);
                if (it == cells.end()) continue;
                for (auto* e : it->second) {
                    if (e != entity && seen.insert(e).second)
                        candidates.push_back(e);
                }
            }
        }
        return candidates;
    }
};

struct WeaponGroup;

struct Unit : Entity {
    double health = 100;
    std::shared_ptr<WeaponGroup> weapon;
    std::vector<ItemTypes> inventory;

    Unit();
    ~Unit() override = default;
    void update() override;
};

struct Player : Unit {
    int player_id = 0;
    std::string name;
    std::vector<int> pressedKeyList;
    std::vector<double> joystickAxisList;
    std::vector<ItemTypes> gottenItem;
    int magabombQuantity = 1;
    bool isReady = false;
    bool isThrowingMagabomb = false;

    explicit Player(int player_id_);
};

struct Enemy : Unit {
    std::optional<Point> targetPos;
    static std::mt19937 rng;

    Enemy();
    void update() override;
    void randomTargetPos();
    void faceToTargetPos();
    double distanceToTargetPos() const;
};

// ---- Weapon ----

struct Projectile;

struct Weapon {
    std::shared_ptr<Projectile> bulletPrototype;
    double fireRate = 10;
    double cooldown = 0;
    int level = 1;
    int maxLevel = 10;
    Race shooterRace = Race::neutral;
    Sounds sound = Sounds::shotgun_shoot;
    bool playSound = true;
    bool isShooting = false;
    WeaponJamType jamType = WeaponJamType::none;

    Weapon() = default;
    Weapon(std::shared_ptr<Projectile> bp, double fr, Race sr);
    virtual ~Weapon() = default;

    virtual std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1);
    void upgrade() { if (level < maxLevel) ++level; }
    void update() { if (cooldown > 0) --cooldown; }
};

struct Projectile : Entity {
    double damage = 10;
    double lifetime = 1000;
    Race shooterRace = Race::neutral;
    bool chooseTarget = false;
    double chooseTargetAngle = 360;
    std::shared_ptr<Entity> target;

    Projectile() = default;
    Projectile(double dmg, double life);
    void update() override;
    void onCollision(Entity& other) override;
};

struct WeaponGroup {
    std::vector<std::shared_ptr<Weapon>> weapons;
    bool isShooting_ = false;

    WeaponGroup() = default;
    explicit WeaponGroup(std::vector<std::shared_ptr<Weapon>>&& ws) : weapons(std::move(ws)) {}

    void addWeapon(std::shared_ptr<Weapon> w) { weapons.push_back(std::move(w)); }
    bool has(const std::type_info& type) const;
    void removeAll(const std::type_info& type);
    void upgrade(const std::type_info& type);
    int getHighestLevel() const;

    bool isShooting() const { return isShooting_; }
    void setShooting(bool v);

    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y);
    void update();
};

// ---- Concrete Projectiles ----

struct Bullet : Projectile {
    Bullet();
};
struct Lazer : Projectile {
    Lazer();
};
struct AutocannonShells : Projectile {
    AutocannonShells();
};
struct Missile : Projectile {
    int handedness = 1;
    int lockTargetDelay = static_cast<int>(0.5 * GAME_TICK);
    int awaitedDelay = static_cast<int>(1.5 * GAME_TICK);

    Missile(int handedness_ = 1);
    void update() override;
};
struct Rocket : Projectile {
    int handedness = 1;
    Rocket(int handedness_ = 1);
};
struct EnergyBall : Projectile {
    EnergyBall();
};
struct EnergyBallEnhanced : Projectile {
    EnergyBallEnhanced();
};
struct RocketEnemy : Projectile {
    RocketEnemy();
};
struct Magabomb : Projectile {
    double explodeX = SCREEN_W / 2.0;
    double explodeY = SCREEN_H / 2.0;
    bool isExploding = false;

    Magabomb();
    void explode();
    void update() override;
    void onCollision(Entity&) override {}
};

// ---- Concrete Weapons ----

struct Shotgun : Weapon {
    static const std::unordered_map<int, std::vector<std::variant<int, std::pair<int,int>>>> bulletSpreadMap;
    Shotgun(Race sr);
    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1) override;
};
struct LazerGun : Weapon {
    LazerGun(Race sr);
    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1) override;
};
struct Autocannon : Weapon {
    Autocannon(Race sr);
    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1) override;
};
struct MissileLauncher : Weapon {
    MissileLauncher(Race sr);
    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1) override;
};
struct RocketLauncher : Weapon {
    RocketLauncher(Race sr);
    std::vector<std::shared_ptr<Projectile>> shoot(double x, double y, int times = 1) override;
};
struct EnergyWeapon : Weapon {
    EnergyWeapon(Race sr);
};
struct EnergyWeaponEnhanced : Weapon {
    EnergyWeaponEnhanced(Race sr);
};
struct RocketLauncherEnemy : Weapon {
    RocketLauncherEnemy(Race sr);
};
struct Shotgun_slow : Shotgun { Shotgun_slow(Race sr); };
struct Shotgun_normal : Shotgun { Shotgun_normal(Race sr); };
struct MissileLauncher_slow : MissileLauncher { MissileLauncher_slow(Race sr); };
