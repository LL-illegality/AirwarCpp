#include "Particle.h"
#include "../Core/RNG.h"
#include <cmath>
#include <algorithm>

static std::mt19937& prng() { return globalRNG(); }
static float pf(float a, float b) { 
    if (a > b) std::swap(a, b);
    if (a == b) return a;
    return std::uniform_real_distribution<float>(a, b)(prng()); 
}
static int pi(int a, int b) { 
    if (a > b) std::swap(a, b);
    if (a == b) return a;
    return std::uniform_int_distribution<int>(a, b)(prng()); 
}

void Particle::update() {
    if (!alive) return;
    ++age;
    if (age >= lifetime) { alive = false; return; }
    float tt = t();
    prevSize = size; prevR = r; prevG = g; prevB = b; prevA = a;
    vx += ax; vy += ay;
    x += vx; y += vy;
    angle += angularVel;

    // Interpolate size: from start->end
    float sizeEnd = 0;
    size = prevSize + (sizeEnd - prevSize) * (1.0f / lifetime);

    // Simple color fade
    a = (Uint8)(255 * (1.0f - tt));
}

void Particle::draw(SDL_Renderer* renderer, float ox, float oy) const {
    if (!alive || a == 0) return;
    float px = x + ox, py = y + oy;
    int sz = std::max(1, (int)size);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);

    if (shape == 0) { // circle
        for (int dy = -sz; dy <= sz; ++dy)
            for (int dx = -sz; dx <= sz; ++dx)
                if (dx * dx + dy * dy <= sz * sz)
                    SDL_RenderPoint(renderer, px + dx, py + dy);
    } else if (shape == 1) { // rect
        SDL_FRect rect = { px - sz / 2, py - sz / 2, (float)sz, (float)sz };
        SDL_RenderFillRect(renderer, &rect);
    } else if (shape == 2) { // triangle
        // Simplified: draw as small circle
        for (int dy = -sz; dy <= sz; ++dy)
            for (int dx = -sz; dx <= sz; ++dx)
                if (dx * dx + dy * dy <= sz * sz)
                    SDL_RenderPoint(renderer, px + dx, py + dy);
    }
}

void ParticleGroup::emit(int count, float ox, float oy) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.lifetime = std::max(1, pi(config.lifetime.first, config.lifetime.second));
        p.x = x + ox + (spawnW > 0 ? pf(-spawnW / 2, spawnW / 2) : 0);
        p.y = y + oy + (spawnH > 0 ? pf(-spawnH / 2, spawnH / 2) : 0);
        p.vx = config.vxRange > 0 ? pf(-config.vxRange, config.vxRange) : 0;
        p.vy = config.vyRange > 0 ? pf(-config.vyRange, config.vyRange) : 0;
        p.size = pf(config.size.first, config.size.second);
        p.r = (Uint8)pi(config.rRange.first, config.rRange.second);
        p.g = (Uint8)pi(config.gRange.first, config.gRange.second);
        p.b = (Uint8)pi(config.bRange.first, config.bRange.second);
        p.a = (Uint8)pi(config.aRange.first, config.aRange.second);
        p.ay += gravity;
        p.ax += wind;
        p.shape = config.shape;
        particles.push_back(p);
    }
}

void ParticleGroup::update() {
    if (!active) return;
    if (emissionRate > 0 && !oneShot) {
        // Continuous emission for trails
    }
    for (auto& p : particles) p.update();
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](auto& p) { return !p.alive; }), particles.end());

    if (damping != 1.0f) {
        for (auto& p : particles) { p.vx *= damping; p.vy *= damping; }
    }
}

void ParticleGroup::draw(SDL_Renderer* renderer, float ox, float oy) {
    for (auto& p : particles) p.draw(renderer, ox + x, oy + y);
}

// ---- Concrete effects ----

EnemyExplosion::EnemyExplosion(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 10; spawnH = 10;
    gravity = 0.05f; damping = 0.97f;
    config = {{20, 40}, {4, 0}, {255, 80}, {200, 20}, {50, 0}, {255, 0}, 3, 3};
    emit(pi(15, 25));
}

PlayerExplosion::PlayerExplosion(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 30; spawnH = 30;
    gravity = 0.05f; damping = 0.97f;
    config = {{30, 60}, {8, 0}, {255, 255}, {255, 100}, {200, 0}, {255, 0}, 6, 6};
    emit(pi(30, 40));
    config = {{40, 80}, {5, 1}, {200, 50}, {200, 50}, {200, 50}, {150, 0}, 4, 4};
    emit(pi(15, 25));
    config = {{10, 25}, {2, 0}, {255, 255}, {255, 200}, {255, 0}, {255, 0}, 10, 10, 1};
    emit(pi(10, 15));
}

MissileTrail::MissileTrail(float x, float y) {
    this->x = x; this->y = y;
    oneShot = false; emissionRate = 0; maxParticles = 30;
    gravity = 0.02f;
    config = {{15, 30}, {3, 0.5f}, {180, 80}, {180, 80}, {180, 80}, {200, 0}, 0, 0};
}

RocketTrail::RocketTrail(float x, float y) {
    this->x = x; this->y = y;
    oneShot = false; emissionRate = 0; maxParticles = 30;
    gravity = 0.02f;
    config = {{15, 30}, {3, 0.5f}, {255, 100}, {200, 30}, {50, 0}, {200, 0}, 0, 0};
}

MissileHit::MissileHit(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 8; spawnH = 8;
    gravity = 0.08f; damping = 0.96f;
    config = {{15, 25}, {3, 0}, {255, 135}, {255, 216}, {255, 255}, {255, 0}, 4, 4};
    emit(pi(10, 15));
}

RocketHit::RocketHit(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 8; spawnH = 8;
    gravity = 0.08f; damping = 0.96f;
    config = {{15, 25}, {3, 0}, {255, 80}, {200, 20}, {80, 0}, {255, 0}, 4, 4};
    emit(pi(20, 30));
}

NukeExplosion::NukeExplosion(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 30; spawnH = 30;
    config = {{15, 15}, {350, 0}, {255, 255}, {255, 255}, {255, 255}, {200, 0}, 0, 0};
    emit(1);
    config = {{30, 90}, {10, 0}, {255, 200}, {255, 50}, {200, 0}, {255, 0}, 8, 8};
    emit(120);
    config = {{30, 90}, {5, 1}, {255, 100}, {200, 30}, {100, 0}, {200, 0}, 12, 12};
    emit(80);
    config = {{60, 120}, {15, 3}, {150, 30}, {150, 30}, {150, 30}, {180, 0}, 3, 4};
    emit(100);
    config = {{50, 100}, {4, 1}, {200, 80}, {200, 80}, {150, 50}, {255, 0}, 15, 15, 1};
    emit(60);
    config = {{80, 150}, {2, 0}, {180, 100}, {180, 100}, {180, 100}, {100, 0}, 3, 2};
    emit(40);
}

BulletHit::BulletHit(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 4; spawnH = 4;
    config = {{8, 15}, {3, 0}, {255, 180}, {200, 120}, {50, 20}, {255, 0}, 2, 2};
    emit(pi(4, 7));
}

LazerHit::LazerHit(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 4; spawnH = 4;
    config = {{8, 15}, {3, 0}, {100, 30}, {200, 100}, {255, 200}, {255, 0}, 2, 2};
    emit(pi(4, 7));
}

AutocannonHit::AutocannonHit(float x, float y) {
    this->x = x; this->y = y;
    oneShot = true; spawnW = 4; spawnH = 4;
    config = {{8, 15}, {5, 0}, {255, 200}, {150, 80}, {150, 80}, {255, 0}, 3, 3};
    emit(pi(4, 7));
}
