#include <cstdio>
#include <cstdlib>
#include <string>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#undef min
#undef max
#endif
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cmath>
#include "../Core/RNG.h"
#include "../Core/Constants.h"
#include "../Render/TextureCache.h"
#include "../Render/SpriteRenderer.h"
#include "../Render/Background.h"
#include "../Render/Particle.h"
#include "../UI/HUD.h"
#include "../UI/InputHandler.h"


static std::string getExeDir() {
    const char* base = SDL_GetBasePath();
    if (!base) return "";
    std::string p = base;
    SDL_free(const_cast<char*>(base));
    return p;
}

int main(int, char**) {
    std::string root = getExeDir();
#ifdef _WIN32
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

    printf("AirwarCPP Client\n");
    printf("  exe dir: %s\n", root.c_str());

    seedRNG();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError()); return 1;
    }
    TTF_Init();

    SDL_Window* win = SDL_CreateWindow("Airwar", 800, 664, 0);
    if (!win) { printf("Window failed: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, NULL);
    if (!ren) { printf("Renderer failed: %s\n", SDL_GetError()); return 1; }

    TextureCache texCache;
    texCache.init(ren);
    SpriteRenderer spr;
    spr.init(ren);
    HUD hud;

    // Load font
    std::string fontWin = "C:\\Windows\\Fonts\\arial.ttf";
    hud.init(ren, &texCache, &spr, fontWin, 800, 664, root);

    // Load textures
    texCache.load(root + "Resources\\images\\player1.png");
    texCache.load(root + "Resources\\images\\en.png");
    texCache.load(root + "Resources\\images\\missile.png");
    texCache.load(root + "Resources\\images\\item_maga.png");

    // Background
    Background bg;
    bg.init(ren, 800, 600);

    InputHandler input;

    Uint64 start = SDL_GetTicks();
    bool running = true;
    int frameCount = 0;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                input.handleKeyDown(e.key.key);
                if (e.key.key == SDLK_ESCAPE) running = false;
            }
            if (e.type == SDL_EVENT_KEY_UP) input.handleKeyUp(e.key.key);
        }

        float t = (SDL_GetTicks() - start) / 1000.0f;

        SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
        SDL_RenderClear(ren);
        bg.update(); bg.draw();

        // Animate a player sprite
        float px = 400 + 150 * sinf(t * 0.5f);
        float py = 350 + 80 * cosf(t * 0.3f);
        auto* pTex = texCache.load(root + "Resources\\images\\player1.png");
        spr.draw(pTex, px, py, t * 30);

        // Animate an enemy
        float ex = 500 + 100 * sinf(t * 0.4f);
        float ey = 150 + 60 * cosf(t * 0.5f);
        spr.draw(texCache.load(root + "Resources\\images\\en.png"), ex, ey, 90 + t * 20);

        // HUD
        hud.updateEntity(0, px, py, t * 30, 100, "player1", "Player1", 0, 3, true);
        hud.update();
        hud.draw(0);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
        ++frameCount;

        // Auto-exit after 10 seconds for testing
        if (SDL_GetTicks() - start > 10000) running = false;
    }

    texCache.clear();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    // SDL_Quit skipped to avoid DLL unloading crash
    return 0;
}
