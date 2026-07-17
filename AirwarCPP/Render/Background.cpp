#include "Background.h"
#include "../Core/RNG.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

// ---- Constants (mapped from Python const.py) ----
static constexpr float SCROLL_SPEED = 0.3f;
static constexpr int BG_REMOVE_MARGIN = 80;
static constexpr int BG_VISIBLE_MARGIN = 30;

static constexpr Uint8 SKY_TOP_R = 90, SKY_TOP_G = 160, SKY_TOP_B = 215;
static constexpr Uint8 SKY_BOT_R = 35, SKY_BOT_G = 90, SKY_BOT_B = 150;

static constexpr int CLOUD_GRAY_MIN = 192, CLOUD_GRAY_MAX = 255;
static constexpr int CLOUD_ALPHA_MIN = 20, CLOUD_ALPHA_MAX = 120;
static constexpr int CLOUD_SPAWN_X_OFFSET = 50;
static constexpr int CLOUD_Y_ABOVE_MIN = 10, CLOUD_Y_ABOVE_MAX = 60;
static constexpr int CLOUD_BLOCK_COUNT_MIN = 3, CLOUD_BLOCK_COUNT_MAX = 8;
static constexpr int CLOUD_BLOCK_W_MIN = 10, CLOUD_BLOCK_W_MAX = 35;
static constexpr int CLOUD_BLOCK_H_MIN = 6, CLOUD_BLOCK_H_MAX = 18;
static constexpr int CLOUD_BLOCK_X_MIN = -25, CLOUD_BLOCK_X_MAX = 25;
static constexpr int CLOUD_BLOCK_Y_MIN = -20, CLOUD_BLOCK_Y_MAX = 20;
static constexpr float CLOUD_SPEED_MIN = 0.3f, CLOUD_SPEED_MAX = 1.2f;
static constexpr float CLOUD_SPAWN_CHANCE = 0.025f;
static constexpr int CLOUD_INITIAL_COUNT = 10;

static constexpr int RIVER_COUNT_MIN = 1, RIVER_COUNT_MAX = 3;
static constexpr int RIVER_NUM_POINTS = 10000;
static constexpr float RIVER_START_Y = -200000.0f;
static constexpr float RIVER_X_STEP_MIN = -50, RIVER_X_STEP_MAX = 50;
static constexpr float RIVER_Y_STEP_MIN = 15, RIVER_Y_STEP_MAX = 30;
static constexpr Uint8 RIVER_GRAY_MIN = 192, RIVER_GRAY_MAX = 200;
static constexpr float RIVER_WIDTH_MIN = 2, RIVER_WIDTH_MAX = 10;
static constexpr Uint8 RIVER_ALPHA_MIN = 20, RIVER_ALPHA_MAX = 80;
static constexpr int RIVER_DRAW_STEP = 10;
static constexpr float RIVER_X_OFFSET = 30;
static constexpr float RIVER_X_CLAMP_MIN = 10;
static constexpr int RIVER_LIFE_MIN = 1800, RIVER_LIFE_MAX = 5400;
static constexpr int RIVER_FADE_DURATION = 600;
static constexpr float RIVER_SPAWN_CHANCE = 0.0008f;
static constexpr float RIVER_SPAWN_Y_ABOVE = 200;
static constexpr int RIVER_FADE_IN_DURATION = 600;

static constexpr int MOUNTAIN_X_MARGIN = 20;
static constexpr int MOUNTAIN_Y_ABOVE_MIN = 20, MOUNTAIN_Y_ABOVE_MAX = 60;
static constexpr float MOUNTAIN_COLLISION_DIST = 25;
static constexpr int MOUNTAIN_COLLISION_IDX_RANGE = 2;
static constexpr int MOUNTAIN_SIZE_W_MIN = 30, MOUNTAIN_SIZE_W_MAX = 80;
static constexpr int MOUNTAIN_SIZE_H_MIN = 20, MOUNTAIN_SIZE_H_MAX = 50;
static constexpr float MOUNTAIN_SCALE_MIN = 0.5f, MOUNTAIN_SCALE_MAX = 2.5f;
static constexpr float MOUNTAIN_BIG_CHANCE = 0.15f;
static constexpr float MOUNTAIN_BIG_SCALE_W_MIN = 1.8f, MOUNTAIN_BIG_SCALE_W_MAX = 3.0f;
static constexpr float MOUNTAIN_BIG_SCALE_H_MIN = 1.5f, MOUNTAIN_BIG_SCALE_H_MAX = 2.5f;
static constexpr Uint8 MOUNTAIN_COLOR_MIN = 30, MOUNTAIN_COLOR_MAX = 60;
static constexpr Uint8 MOUNTAIN_MAX_ALPHA = 20;
static constexpr float MOUNTAIN_ALPHA_OFFSET_MIN = 1.0f, MOUNTAIN_ALPHA_OFFSET_MAX = 3.0f;
static constexpr float MOUNTAIN_SPAWN_CHANCE = 0.008f;
static constexpr int MOUNTAIN_INITIAL_COUNT = 12;

static std::mt19937& rng() { return globalRNG(); }
template<typename T>
static T rngRange(T min, T max) { return std::uniform_int_distribution<T>(min, max)(rng()); }
template<>
float rngRange<float>(float min, float max) { return std::uniform_real_distribution<float>(min, max)(rng()); }
static bool rngChance(float p) { return std::uniform_real_distribution<float>(0, 1)(rng()) < p; }

Background::~Background() {
    if (skyTexture_) SDL_DestroyTexture(skyTexture_);
    for (auto& c : clouds_) if (c.texture) SDL_DestroyTexture(c.texture);
    for (auto& m : mountains_) if (m.texture) SDL_DestroyTexture(m.texture);
}

void Background::init(SDL_Renderer* r, int w, int h) {
    renderer_ = r;
    width_ = w; height_ = h;
    scrollSpeed_ = SCROLL_SPEED;
    createSky();
    generateRivers();
    for (int i = 0; i < CLOUD_INITIAL_COUNT; ++i) {
        spawnCloud();
        if (!clouds_.empty()) {
            clouds_.back().x = rngRange<float>(-CLOUD_SPAWN_X_OFFSET, width_ + CLOUD_SPAWN_X_OFFSET);
            clouds_.back().y = rngRange<float>(0, height_);
        }
    }
    for (int i = 0; i < MOUNTAIN_INITIAL_COUNT; ++i) {
        spawnMountain();
        if (!mountains_.empty())
            mountains_.back().y = rngRange<float>(0, height_);
    }
}

void Background::createSky() {
    auto* surf = SDL_CreateSurface(width_, height_, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return;
    SDL_LockSurface(surf);
    auto* pixels = (Uint32*)surf->pixels;
    for (int y = 0; y < height_; ++y) {
        float t = (float)y / height_;
        Uint8 r = (Uint8)(SKY_TOP_R + (SKY_BOT_R - SKY_TOP_R) * t);
        Uint8 g = (Uint8)(SKY_TOP_G + (SKY_BOT_G - SKY_TOP_G) * t);
        Uint8 b = (Uint8)(SKY_TOP_B + (SKY_BOT_B - SKY_TOP_B) * t);
        Uint32 color = (255u << 24) | (b << 16) | (g << 8) | r;
        for (int x = 0; x < width_; ++x)
            pixels[y * surf->w + x] = color;
    }
    SDL_UnlockSurface(surf);
    skyTexture_ = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_DestroySurface(surf);
}

void Background::spawnCloud() {
    Uint8 gray = rngRange<int>(CLOUD_GRAY_MIN, CLOUD_GRAY_MAX);
    Uint8 alpha = rngRange<int>(CLOUD_ALPHA_MIN, CLOUD_ALPHA_MAX);

    struct Block { int x, y, w, h; };
    std::vector<Block> blocks;
    int n = rngRange<int>(CLOUD_BLOCK_COUNT_MIN, CLOUD_BLOCK_COUNT_MAX);
    int minBX = 99999, minBY = 99999, maxBX = -99999, maxBY = -99999;
    for (int i = 0; i < n; ++i) {
        int bw = rngRange<int>(CLOUD_BLOCK_W_MIN, CLOUD_BLOCK_W_MAX);
        int bh = rngRange<int>(CLOUD_BLOCK_H_MIN, CLOUD_BLOCK_H_MAX);
        int bx = rngRange<int>(CLOUD_BLOCK_X_MIN, CLOUD_BLOCK_X_MAX);
        int by = rngRange<int>(CLOUD_BLOCK_Y_MIN, CLOUD_BLOCK_Y_MAX);
        blocks.push_back({bx, by, bw, bh});
        minBX = std::min(minBX, bx); minBY = std::min(minBY, by);
        maxBX = std::max(maxBX, bx + bw); maxBY = std::max(maxBY, by + bh);
    }
    int cw = maxBX - minBX, ch = maxBY - minBY;
    if (cw <= 0 || ch <= 0) return;

    auto* surf = SDL_CreateSurface(cw, ch, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return;
    SDL_LockSurface(surf);
    auto* pix = (Uint32*)surf->pixels;
    Uint32 color = ((Uint32)alpha << 24) | (gray << 16) | (gray << 8) | gray;
    for (auto& b : blocks) {
        for (int dy = 0; dy < b.h; ++dy)
            for (int dx = 0; dx < b.w; ++dx) {
                int px = b.x - minBX + dx, py = b.y - minBY + dy;
                if (px >= 0 && px < cw && py >= 0 && py < ch)
                    pix[py * cw + px] = color;
            }
    }
    SDL_UnlockSurface(surf);
    auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_DestroySurface(surf);

    float cx = rngRange<float>(-CLOUD_SPAWN_X_OFFSET, width_ + CLOUD_SPAWN_X_OFFSET);
    float cy = -rngRange<float>(CLOUD_Y_ABOVE_MIN, CLOUD_Y_ABOVE_MAX);
    float speed = rngRange<float>(CLOUD_SPEED_MIN, CLOUD_SPEED_MAX);
    clouds_.push_back({cx, cy, speed, tex, cw, ch});
}

void Background::generateRivers() {
    int count = rngRange<int>(RIVER_COUNT_MIN, RIVER_COUNT_MAX);
    float spacing = (float)width_ / (count + 1);
    for (int i = 0; i < count; ++i) {
        float x = spacing * (i + 1) + rngRange<float>(-RIVER_X_OFFSET, RIVER_X_OFFSET);
        spawnNewRiver();
        if (!rivers_.empty()) {
            rivers_.back().ptsX[0] = x;
            int rp = RIVER_NUM_POINTS;
            float cx = x;
            for (int j = 1; j < rp; ++j) {
                cx += rngRange<float>(RIVER_X_STEP_MIN, RIVER_X_STEP_MAX);
                cx = std::max(RIVER_X_CLAMP_MIN, std::min((float)width_ - RIVER_X_CLAMP_MIN, cx));
                rivers_.back().ptsX[j] = cx;
            }
        }
    }
}

void Background::spawnNewRiver() {
    float cx = rngRange<float>(RIVER_X_CLAMP_MIN, width_ - RIVER_X_CLAMP_MIN);
    River r;
    r.numPoints = RIVER_NUM_POINTS;
    r.ptsX.resize(RIVER_NUM_POINTS);
    r.ptsY.resize(RIVER_NUM_POINTS);
    float cy = RIVER_START_Y;
    for (int i = 0; i < RIVER_NUM_POINTS; ++i) {
        r.ptsX[i] = cx;
        r.ptsY[i] = cy;
        cx += rngRange<float>(RIVER_X_STEP_MIN, RIVER_X_STEP_MAX);
        cx = std::max(RIVER_X_CLAMP_MIN, std::min((float)width_ - RIVER_X_CLAMP_MIN, cx));
        cy += rngRange<float>(RIVER_Y_STEP_MIN, RIVER_Y_STEP_MAX);
    }
    r.gray = rngRange<int>(RIVER_GRAY_MIN, RIVER_GRAY_MAX);
    r.life = r.maxLife = rngRange<int>(RIVER_LIFE_MIN, RIVER_LIFE_MAX);
    r.width = rngRange<float>(RIVER_WIDTH_MIN, RIVER_WIDTH_MAX);
    r.alpha = rngRange<int>(RIVER_ALPHA_MIN, RIVER_ALPHA_MAX);
    rivers_.push_back(std::move(r));
}

void Background::spawnMountain() {
    int x = rngRange<int>(MOUNTAIN_X_MARGIN, width_ - MOUNTAIN_X_MARGIN);

    float scale = rngRange<float>(MOUNTAIN_SCALE_MIN, MOUNTAIN_SCALE_MAX);
    int w = (int)(rngRange<int>(MOUNTAIN_SIZE_W_MIN, MOUNTAIN_SIZE_W_MAX) * scale);
    int h = (int)(rngRange<int>(MOUNTAIN_SIZE_H_MIN, MOUNTAIN_SIZE_H_MAX) * scale);
    if (rngChance(MOUNTAIN_BIG_CHANCE)) {
        w = (int)(w * rngRange<float>(MOUNTAIN_BIG_SCALE_W_MIN, MOUNTAIN_BIG_SCALE_W_MAX));
        h = (int)(h * rngRange<float>(MOUNTAIN_BIG_SCALE_H_MIN, MOUNTAIN_BIG_SCALE_H_MAX));
    }
    if (w <= 0 || h <= 0) return;

    Uint8 c = rngRange<int>(MOUNTAIN_COLOR_MIN, MOUNTAIN_COLOR_MAX);
    float aoff = rngRange<float>(MOUNTAIN_ALPHA_OFFSET_MIN, MOUNTAIN_ALPHA_OFFSET_MAX);

    auto* surf = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA8888);
    if (!surf) return;
    SDL_LockSurface(surf);
    auto* pix = (Uint32*)surf->pixels;
    float half = w / 2.0f;
    for (int px = 0; px < w; ++px) {
        float dist = std::abs(px - half) / half;
        int hh = (int)((1.0f - dist) * h);
        if (hh <= 0) continue;
        Uint8 a = (Uint8)std::min(255.0f, (px / (float)w) * MOUNTAIN_MAX_ALPHA * aoff);
        Uint32 color = ((Uint32)a << 24) | (c << 16) | (c << 8) | c;
        for (int py = h - hh; py < h; ++py)
            pix[py * w + px] = color;
    }
    SDL_UnlockSurface(surf);
    auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
    SDL_DestroySurface(surf);

    float my = -rngRange<float>(MOUNTAIN_Y_ABOVE_MIN, MOUNTAIN_Y_ABOVE_MAX);
    mountains_.push_back({(float)x, my, tex, w, h});
}

void Background::update() {
    int margin = BG_REMOVE_MARGIN;
    for (auto it = clouds_.begin(); it != clouds_.end(); ) {
        it->y += it->speed;
        if (it->y > height_ + margin) it = clouds_.erase(it);
        else ++it;
    }
    if (rngChance(CLOUD_SPAWN_CHANCE)) spawnCloud();

    for (auto it = rivers_.begin(); it != rivers_.end(); ) {
        it->offset += scrollSpeed_;
        it->life -= 1;
        if (it->life <= 0 && rivers_.size() > RIVER_COUNT_MIN) it = rivers_.erase(it);
        else ++it;
    }
    if (rivers_.size() < RIVER_COUNT_MAX && rngChance(RIVER_SPAWN_CHANCE))
        spawnNewRiver();

    for (auto it = mountains_.begin(); it != mountains_.end(); ) {
        it->y += scrollSpeed_;
        if (it->y > height_ + margin) it = mountains_.erase(it);
        else ++it;
    }
    if (rngChance(MOUNTAIN_SPAWN_CHANCE)) spawnMountain();
}

void Background::drawRiver(SDL_Texture* surf, River& river) {
    int margin = BG_VISIBLE_MARGIN;
    std::vector<SDL_FPoint> pts;
    auto& ptsX = river.ptsX;
    auto& ptsY = river.ptsY;
    float offset = river.offset;

    for (int sy = -margin; sy <= height_ + margin; sy += RIVER_DRAW_STEP) {
        float ry = sy - offset;
        auto it = std::lower_bound(ptsY.begin(), ptsY.end(), ry);
        int idx = (int)std::distance(ptsY.begin(), it);
        idx = std::max(1, std::min(idx, (int)ptsX.size() - 1));
        float y0 = ptsY[idx - 1], y1 = ptsY[idx];
        float x;
        if (y1 > y0) {
            float f = (ry - y0) / (y1 - y0);
            x = ptsX[idx - 1] + (ptsX[idx] - ptsX[idx - 1]) * f;
        } else {
            x = ptsX[idx - 1];
        }
        pts.push_back({x, (float)sy});
    }
    if (pts.size() < 2) return;

    int baseAlpha = river.alpha;
    int elapsed = river.maxLife - river.life;
    if (elapsed < RIVER_FADE_IN_DURATION)
        baseAlpha = (int)(baseAlpha * elapsed / RIVER_FADE_IN_DURATION);
    if (river.life < RIVER_FADE_DURATION)
        baseAlpha = (int)(baseAlpha * river.life / RIVER_FADE_DURATION);
    baseAlpha = std::max(0, std::min(255, baseAlpha));

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, river.gray, river.gray, river.gray, (Uint8)baseAlpha);
    for (size_t i = 1; i < pts.size(); ++i) {
        SDL_RenderLine(renderer_, pts[i-1].x, pts[i-1].y, pts[i].x, pts[i].y);
    }
}

void Background::draw() {
    SDL_RenderTexture(renderer_, skyTexture_, NULL, NULL);

    for (auto& m : mountains_) {
        SDL_FRect dst = { m.x - m.texW / 2.0f, m.y, (float)m.texW, (float)m.texH };
        SDL_RenderTexture(renderer_, m.texture, NULL, &dst);
    }

    for (auto& r : rivers_)
        drawRiver(nullptr, r);

    for (auto& c : clouds_) {
        SDL_FRect dst = { c.x, c.y, (float)c.texW, (float)c.texH };
        SDL_RenderTexture(renderer_, c.texture, NULL, &dst);
    }
}
