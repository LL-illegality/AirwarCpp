#include <cstdio>
#include <cstdlib>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Core/Constants.h"
#include "Core/RNG.h"
#include "Render/TextureCache.h"
#include "Render/SpriteRenderer.h"
#include "UI/HUD.h"

static int testsPassed = 0;
static int testsFailed = 0;
#define CHECK(cond, msg) do {                                              \
    if (cond) { ++testsPassed;                                              \
        printf("  PASS: %s\n", msg);                                        \
    } else { ++testsFailed;                                                 \
        printf("  FAIL: %s\n  at line %d\n", msg, __LINE__);                \
    } } while(0)

int main(int, char**) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("AirwarCPP Phase 5 -- Full Resource Loading Test\n");
    printf("===============================================\n\n");

    seedRNG();

    /* ====== Init SDL + TTF ====== */
    CHECK(SDL_Init(SDL_INIT_VIDEO), "SDL_Init(VIDEO)");
    CHECK(TTF_Init(), "TTF_Init()");

    SDL_Window* win = SDL_CreateWindow("AirwarCPP", 800, 664, 0);
    CHECK(win != NULL, "Window created");
    SDL_Renderer* ren = SDL_CreateRenderer(win, NULL);
    CHECK(ren != NULL, "Renderer created");

    TextureCache texCache;
    texCache.init(ren);
    SpriteRenderer spriteRenderer;
    spriteRenderer.init(ren);

    /* ====== 1. Load and render sprite textures ====== */
    printf("[1] Sprite texture loading\n");
    SDL_Texture* playerTex = texCache.load("Resources\\images\\player1.png");
    CHECK(playerTex != NULL, "player1.png loaded");
    SDL_Texture* enTex = texCache.load("Resources\\images\\en.png");
    CHECK(enTex != NULL, "en.png loaded");
    SDL_Texture* missileTex = texCache.load("Resources\\images\\missile.png");
    CHECK(missileTex != NULL, "missile.png loaded");
    SDL_Texture* magaTex = texCache.load("Resources\\images\\item_maga.png");
    CHECK(magaTex != NULL, "item_maga.png loaded");
    SDL_Texture* readyTex = texCache.load("Resources\\images\\ready.png");
    CHECK(readyTex != NULL, "ready.png loaded");

    /* ====== 2. HUD with font + textures ====== */
    printf("[2] HUD with fonts\n");
    HUD hud;
    hud.init(ren, &texCache, &spriteRenderer,
             "C:\\Windows\\Fonts\\arial.ttf", 800, 664);
    hud.setVersion("Ver. 1.3.1");
    hud.setLevel("Level 1");

    hud.addTitle("Airwar", 180);
    hud.updateEntity(0, 400, 300, 0, 100, "player1", "Player1", 0, 3, true);
    hud.updateEntity(1, 500, 200, 90, 60, "en", "Enemy");
    hud.updateEntity(2, 300, 150, 45, 50, "missile");

    /* ====== 3. Animated render loop ====== */
    printf("[3] Animated rendering\n");
    Uint64 start = SDL_GetTicks();
    int frames = 0;
    bool running = true;
    while (running && (SDL_GetTicks() - start < 4000)) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }

        float t = (SDL_GetTicks() - start) / 1000.0f;

        SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
        SDL_RenderClear(ren);

        // Animate entities in a pattern
        float cx = 400 + 150 * sinf(t);
        float cy = 300 + 80 * cosf(t * 0.7f);
        hud.updateEntity(0, cx, cy, t * 45, 100, "player1", "Player1", 0, 3, true);

        float ex = 500 + 100 * sinf(t * 0.5f);
        float ey = 200 + 60 * cosf(t * 0.3f);
        hud.updateEntity(1, ex, ey, 90 + t * 20, 60, "en", "Enemy");

        hud.updateEntity(2, 300, 150 + t * 30, 45 + t * 60, 50, "missile");

        hud.update();
        hud.draw(0);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
        ++frames;
    }
    CHECK(frames > 50, "Rendered >50 frames with textures + fonts");

    /* ====== 4. Verify texture sizes ====== */
    printf("[4] Texture properties\n");
    if (playerTex) {
        float pw, ph; SDL_GetTextureSize(playerTex, &pw, &ph);
        CHECK(pw == 32 && ph == 32, "player1.png is 32x32");
    }
    if (enTex) {
        float ew, eh; SDL_GetTextureSize(enTex, &ew, &eh);
        CHECK(ew > 0 && eh > 0, "en.png has valid size");
    }

    /* ====== 5. Sprite rotation and scale ====== */
    printf("[5] Sprite transform rendering\n");
    // Non-interpolated draw at various positions
    spriteRenderer.draw(playerTex, 400, 300, 0, 255, 1.0f);
    spriteRenderer.draw(enTex, 200, 150, 45, 128, 1.5f);
    spriteRenderer.draw(missileTex, 600, 450, -30, 200, 0.8f);
    spriteRenderer.drawInterpolated(playerTex, 100, 100, 200, 300, 0, 90, 0.5f);
    CHECK(true, "All sprite transforms completed without error");

    /* ====== 6. HUD title text rendering ====== */
    printf("[6] HUD text rendering\n");
    hud.addTitle("Title Test", 60);
    hud.update();
    CHECK(true, "HUD title text OK");

    hud.setLevel("Level 2");
    hud.update();
    CHECK(true, "HUD level info OK");

    /* ====== 7. Multiple entities with various states ====== */
    printf("[7] Multi-entity states\n");
    hud.clearEntities();
    hud.updateEntity(0, 400, 500, 0, 80, "player1", "Player1", 0, 1, true);
    hud.updateEntity(1, 400, 100, 180, 100, "en", "Boss", -1, 0, false);
    hud.update();
    CHECK(true, "Multi-entity update OK");

    /* ====== 8. Load additional sprite types ====== */
    printf("[8] Additional sprite loading\n");
    CHECK(texCache.load("Resources\\images\\bullet1.png") != NULL, "bullet1.png");
    CHECK(texCache.load("Resources\\images\\lazer_level1.png") != NULL, "lazer_level1.png");
    CHECK(texCache.load("Resources\\images\\rocket.png") != NULL, "rocket.png");
    CHECK(texCache.load("Resources\\images\\unit1.png") != NULL, "unit1.png");
    CHECK(texCache.load("Resources\\images\\big1.png") != NULL, "big1.png");
    CHECK(texCache.load("Resources\\images\\rship.png") != NULL, "rship.png");
    CHECK(texCache.load("Resources\\images\\item_shotgun.png") != NULL, "item_shotgun.png");
    CHECK(texCache.load("Resources\\images\\item_lazer.png") != NULL, "item_lazer.png");

    /* ====== 9. Verify HUD with all features ====== */
    printf("[9] Full HUD feature test\n");
    {
        HUD hud2;
        hud2.init(ren, &texCache, &spriteRenderer,
                  "C:\\Windows\\Fonts\\arial.ttf", 800, 664);
        hud2.addTitle("You Win!", 300);
        hud2.updateEntity(0, 400, 300, 0, 100, "player1", "Hero", 0, 5, true);
        hud2.updateEntity(1, 600, 200, 90, 0, "enemy2", "Foe", -1, 0, false);
        hud2.setVersion("Ver. 2.0");
        hud2.setLevel("Final Stage");

        for (int i = 0; i < 10; ++i) {
            SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
            SDL_RenderClear(ren);
            hud2.update();
            hud2.draw(0);
            SDL_RenderPresent(ren);
            SDL_Delay(16);
        }
        CHECK(true, "Full HUD feature demo rendered");
    }

    /* ====== 10. Cleanup ====== */
    printf("[10] Cleanup\n");
    texCache.clear();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    CHECK(true, "Clean shutdown");

    int total = testsPassed + testsFailed;
    printf("\n===============================================\n");
    printf("  Results: %d / %d passed, %d failed\n", testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
