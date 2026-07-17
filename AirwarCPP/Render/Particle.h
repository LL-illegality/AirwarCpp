#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <functional>

struct Particle {
    int age = 0, lifetime = 60;
    bool alive = true;
    float x = 0, y = 0, vx = 0, vy = 0, ax = 0, ay = 0;
    float size = 3, prevSize = 3;
    Uint8 r = 255, g = 255, b = 255, a = 255;
    Uint8 prevR = 255, prevG = 255, prevB = 255, prevA = 255;
    float angle = 0, angularVel = 0;
    int shape = 0; // 0=circle, 1=rect, 2=triangle, 3=diamond

    float t() const { return lifetime > 0 ? std::min(1.0f, (float)age / lifetime) : 1.0f; }
    void update();
    void draw(SDL_Renderer* renderer, float ox, float oy) const;
};

class ParticleGroup {
public:
    std::vector<Particle> particles;
    float x = 0, y = 0;
    float spawnW = 0, spawnH = 0;
    int emissionRate = 0;
    int maxParticles = 0;
    float gravity = 0, wind = 0, damping = 1.0f;
    bool active = true, oneShot = false;

    // Particle config with range support
    struct PConfig {
        std::pair<int,int> lifetime = {30, 60};
        std::pair<float,float> size = {3, 0};
        std::pair<Uint8,Uint8> rRange = {255, 200};
        std::pair<Uint8,Uint8> gRange = {255, 100};
        std::pair<Uint8,Uint8> bRange = {255, 50};
        std::pair<Uint8,Uint8> aRange = {255, 0};
        float vxRange = 0, vyRange = 0;
        int shape = 0;
    };
    PConfig config;

    ParticleGroup() = default;
    virtual ~ParticleGroup() = default;

    void emit(int count = 1, float ox = 0, float oy = 0);
    void update();
    void draw(SDL_Renderer* renderer, float ox = 0, float oy = 0);
    bool isDead() const { return oneShot && !particles.empty() && particles.front().age >= particles.front().lifetime; }
    int particleCount() const { return (int)particles.size(); }
    void clear() { particles.clear(); }
};

// Concrete effects
struct EnemyExplosion : ParticleGroup {
    EnemyExplosion(float x, float y);
};
struct PlayerExplosion : ParticleGroup {
    PlayerExplosion(float x, float y);
};
struct MissileTrail : ParticleGroup {
    MissileTrail(float x, float y);
};
struct RocketTrail : ParticleGroup {
    RocketTrail(float x, float y);
};
struct MissileHit : ParticleGroup {
    MissileHit(float x, float y);
};
struct RocketHit : ParticleGroup {
    RocketHit(float x, float y);
};
struct NukeExplosion : ParticleGroup {
    NukeExplosion(float x, float y);
};
struct BulletHit : ParticleGroup {
    BulletHit(float x, float y);
};
struct LazerHit : ParticleGroup {
    LazerHit(float x, float y);
};
struct AutocannonHit : ParticleGroup {
    AutocannonHit(float x, float y);
};
