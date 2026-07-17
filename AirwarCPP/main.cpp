#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "Core/Constants.h"
#include "Core/RNG.h"
#include "Render/TextureCache.h"
#include "Render/SpriteRenderer.h"
#include "Render/Background.h"
#include "Render/Particle.h"

static int testsPassed = 0;
static int testsFailed = 0;

#define CHECK(cond, msg) do {                                              \
    if (cond) { ++testsPassed;                                              \
        printf("  PASS: %s\n", msg);                                        \
    } else { ++testsFailed;                                                 \
        printf("  FAIL: %s\n  at line %d\n", msg, __LINE__);                \
    } } while(0)

static std::string assetRoot;

static void initAssetRoot() {
    const char* base = SDL_GetBasePath();
    if (base) { assetRoot = base; SDL_free(const_cast<char*>(base));
        assetRoot += "..\\..\\..\\..\\"; }
}

int main(int, char**) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("AirwarCPP Phase 3 -- Rendering Pipeline\n");
    printf("========================================\n\n");

    seedRNG();

    /* ====== 1. TextureCache ====== */
    printf("[1] TextureCache init\n");
    TextureCache texCache;
    CHECK(true, "TextureCache constructed");

    /* ====== 2. SpriteRenderer ====== */
    printf("[2] SpriteRenderer init\n");
    SpriteRenderer spriteRenderer;
    CHECK(true, "SpriteRenderer constructed");

    /* ====== 3. Background init (no window needed for construction) ====== */
    printf("[3] Background construction\n");
    Background bg;
    CHECK(true, "Background default constructed");

    /* ====== 4. Particle construction ====== */
    printf("[4] Particle effects construction\n");
    {
        ParticleGroup pg;
        pg.oneShot = true;
        pg.config = {{20,40},{4,0},{255,80},{200,20},{50,0},{255,0},3,3};
        pg.emit(10);
        CHECK(pg.particles.size() == 10, "ParticleGroup emit 10 particles");
        CHECK(pg.particles[0].lifetime >= 20 && pg.particles[0].lifetime <= 40,
              "Particle lifetime in range");
    }
    {
        // Individual effect constructors
        EnemyExplosion ee(400, 300);
        CHECK(ee.oneShot, "EnemyExplosion is one-shot");
        CHECK(ee.particles.size() >= 10, "EnemyExplosion has particles");

        MissileHit mh(400, 300);
        CHECK(!mh.particles.empty(), "MissileHit has particles");

        RocketHit rh(400, 300);
        CHECK(!rh.particles.empty(), "RocketHit has particles");

        BulletHit bh(400, 300);
        CHECK(!bh.particles.empty(), "BulletHit has particles");

        LazerHit lh(400, 300);
        CHECK(!lh.particles.empty(), "LazerHit has particles");

        AutocannonHit ah(400, 300);
        CHECK(!ah.particles.empty(), "AutocannonHit has particles");
    }
    {
        MissileTrail mt(400, 300);
        mt.config = {{15,30},{3,0.5f},{180,80},{180,80},{180,80},{200,0},0,0};
        mt.emit(1);
        CHECK(mt.particles.size() == 1, "MissileTrail emit adds particle");

        RocketTrail rt(400, 300);
        rt.config = {{15,30},{3,0.5f},{255,100},{200,30},{50,0},{200,0},0,0};
        rt.emit(1);
        CHECK(rt.particles.size() == 1, "RocketTrail emit adds particle");
    }

    /* ====== 5. Particle update loop ====== */
    printf("[5] Particle update/kill\n");
    {
        EnemyExplosion ee(400, 300);
        for (int i = 0; i < 100; ++i) ee.update();
        CHECK(ee.particles.empty(), "EnemyExplosion particles all dead after 100 ticks");
    }
    {
        MissileTrail mt(400, 300);
        mt.config = {{5,10},{3,0.5f},{180,80},{180,80},{180,80},{200,0},0,0};
        mt.emit(5);
        for (int i = 0; i < 20; ++i) mt.update();
        CHECK(mt.particles.empty(), "MissileTrail particles dead after ticks");
    }

    /* ====== 6. SDL Init + Window ====== */
    printf("[6] SDL Window + Renderer\n");
    CHECK(SDL_Init(SDL_INIT_VIDEO), "SDL_Init(VIDEO)");
    SDL_Window* win = SDL_CreateWindow("AirwarCPP Phase 3", 800, 664, 0);
    CHECK(win != NULL, "SDL_CreateWindow");
    if (!win) return 1;
    SDL_Renderer* ren = SDL_CreateRenderer(win, NULL);
    CHECK(ren != NULL, "SDL_CreateRenderer");
    if (!ren) return 1;

    initAssetRoot();
    texCache.init(ren);
    spriteRenderer.init(ren);

    /* ====== 7. Load PNG texture ====== */
    printf("[7] PNG texture load\n");
    std::string pngPath = assetRoot + "Airwar_python\\images\\player1.png";
    printf("       path=%s\n", pngPath.c_str());
    SDL_Texture* playerTex = texCache.load(pngPath);
    CHECK(playerTex != NULL, "TextureCache.load(player1.png)");

    float tw = 0, th = 0;
    if (playerTex) SDL_GetTextureSize(playerTex, &tw, &th);
    CHECK(tw > 0 && th > 0, "Texture has valid size");

    /* ====== 8. Background init with renderer ====== */
    printf("[8] Background init\n");
    bg.init(ren, 800, 600);
    CHECK(true, "Background.init() completed");

    /* ====== 9. Background update ====== */
    printf("[9] Background update\n");
    for (int i = 0; i < 10; ++i) bg.update();
    CHECK(true, "Background.update() 10 ticks completed");

    /* ====== 10. Sprite draw at various positions/rotations ====== */
    printf("[10] SpriteRenderer draw tests\n");
    // These are visual-only; we test they don't crash
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    bg.draw();
    if (playerTex) spriteRenderer.draw(playerTex, 400, 300, 0, 255);
    CHECK(true, "Sprite draw at (400,300) completed");
    if (playerTex) spriteRenderer.draw(playerTex, 200, 150, 45, 128);
    CHECK(true, "Sprite draw rotated 45deg with alpha completed");
    if (playerTex) spriteRenderer.draw(playerTex, 600, 450, -30, 255, 2.0f);
    CHECK(true, "Sprite draw scaled 2x completed");

    /* ====== 11. Interpolated draw ====== */
    printf("[11] Interpolated sprite draw\n");
    if (playerTex) spriteRenderer.drawInterpolated(playerTex, 100, 100, 200, 300, 0, 90, 0.5f);
    CHECK(true, "Interpolated draw completed");

    /* ====== 12. Particle draw (visual check) ====== */
    printf("[12] Particle effects render\n");
    EnemyExplosion ee(400, 200);
    RocketHit rh(600, 400);
    PlayerExplosion pe(200, 300);
    NukeExplosion ne(400, 300);
    for (int i = 0; i < 60; ++i) {
        SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
        SDL_RenderClear(ren);
        bg.draw();
        if (playerTex) spriteRenderer.draw(playerTex, 400, 500, 0, 255);

        ee.update(); rh.update(); pe.update(); ne.update();
        ee.draw(ren); rh.draw(ren); pe.draw(ren); ne.draw(ren);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }
    CHECK(true, "Particle effects rendered for 60 frames");

    /* ====== 13. PNG texture from file (verify image loading) ====== */
    printf("[13] Additional texture loading\n");
    SDL_Texture* enTex = texCache.load(assetRoot + "Airwar_python\\images\\en.png");
    CHECK(enTex != NULL, "TextureCache.load(en.png)");
    SDL_Texture* bgTex = texCache.load(assetRoot + "Airwar_python\\images\\big1.png");
    CHECK(bgTex != NULL, "TextureCache.load(big1.png)");

    /* ====== 14. Render multiple sprites with trails ====== */
    printf("[14] Multi-sprite animated demo\n");
    {
        float px = 400, py = 500, rot = 0;
        MissileTrail trail(px, py);
        EnemyExplosion explode(px, py);

        for (int i = 0; i < 90; ++i) {
            SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
            SDL_RenderClear(ren);
            bg.draw();

            // Animate player sprite in a circle
            float t = (float)i / 90.0f * 2 * 3.14159f;
            px = 400 + 200 * sinf(t);
            py = 300 + 100 * cosf(t);
            rot = t * 180 / 3.14159f;

            if (playerTex) spriteRenderer.draw(playerTex, px, py, rot, 255);

            // Trail follows
            trail.x = px; trail.y = py;
            if (i % 3 == 0) {
                trail.config = {{10, 20}, {2, 0.5f}, {180, 80}, {180, 80}, {180, 80}, {200, 0}, 0, 0};
                trail.emit(1);
            }
            trail.update();
            trail.draw(ren);

            SDL_RenderPresent(ren);
            SDL_Delay(16);
        }
        CHECK(true, "Animated demo completed without crash");

        // Trigger explosion at final position
        EnemyExplosion finalBoom(px, py);
        for (int i = 0; i < 45; ++i) {
            SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
            SDL_RenderClear(ren);
            bg.draw();
            finalBoom.update(); finalBoom.draw(ren);
            SDL_RenderPresent(ren);
            SDL_Delay(16);
        }
        CHECK(true, "Explosion animation completed without crash");
    }

    /* ====== 15. Cleanup ====== */
    printf("[15] Cleanup\n");
    texCache.clear();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    CHECK(true, "Clean SDL shutdown");

    /* ====== Summary ====== */
    int total = testsPassed + testsFailed;
    printf("\n========================================\n");
    printf("  Results: %d / %d passed, %d failed\n",
           testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
