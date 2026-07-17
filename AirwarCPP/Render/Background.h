#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <memory>

struct Cloud {
    float x, y, speed;
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

struct River {
    std::vector<float> ptsX, ptsY;
    int numPoints = 0;
    float offset = 0;
    Uint8 gray = 192;
    float width = 4;
    Uint8 alpha = 50;
    int life = 1800, maxLife = 1800;
};

struct Mountain {
    float x, y;
    SDL_Texture* texture = nullptr;
    int texW = 0, texH = 0;
};

class Background {
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* skyTexture_ = nullptr;
    int width_ = 800, height_ = 600;
    float scrollSpeed_ = 0.3f;
    std::vector<Cloud> clouds_;
    std::vector<River> rivers_;
    std::vector<Mountain> mountains_;

    void createSky();
    void spawnCloud();
    void generateRivers();
    void spawnNewRiver();
    void spawnMountain();
    void drawRiver(SDL_Texture* surf, River& river);

public:
    Background() = default;
    ~Background();
    void init(SDL_Renderer* r, int w, int h);
    void update();
    void draw();
};
