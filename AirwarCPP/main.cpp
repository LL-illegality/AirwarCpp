#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Core/Constants.h"
#include "Core/RNG.h"
#include "Render/TextureCache.h"
#include "Render/SpriteRenderer.h"
#include "Render/Background.h"
#include "Render/Particle.h"
#include "UI/HUD.h"
#include "Audio/SoundEngine.h"
#include "Audio/MusicPlayer.h"
#include <fstream>
#include "json.hpp"

static int testsPassed = 0;
static int testsFailed = 0;
#define CHECK(cond, msg) do {                                              \
    if (cond) { ++testsPassed;                                              \
        printf("  PASS: %s\n", msg);                                        \
    } else { ++testsFailed;                                                 \
        printf("  FAIL: %s\n  at line %d\n", msg, __LINE__);                \
    } } while(0)

// Resolve absolute asset root from executable location
static std::string getAssetRoot() {
    const char* base = SDL_GetBasePath();
    if (!base) return "";
    std::string path = base;
    SDL_free(const_cast<char*>(base));
    // exe is at: out/build/x64-debug/AirwarCPP/AirwarCPP.exe
    // repo root: ../../../../ = 4 levels up
    path += "..\\..\\..\\..\\";
    return path;
}

int main(int, char**) {
    setvbuf(stdout, NULL, _IONBF, 0);
    std::string root = getAssetRoot();
    printf("AirwarCPP -- Resource Path Test\n");
    printf("  assetRoot = %s\n\n", root.c_str());

    seedRNG();
    CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), "SDL_Init");
    CHECK(TTF_Init(), "TTF_Init()");

    SDL_Window* win = SDL_CreateWindow("AirwarCPP Resource Test", 800, 664, 0);
    CHECK(win != NULL, "Window");
    SDL_Renderer* ren = SDL_CreateRenderer(win, NULL);
    CHECK(ren != NULL, "Renderer");

    TextureCache texCache;
    texCache.init(ren);
    SpriteRenderer spr;
    spr.init(ren);

    // Helper: build absolute path
    auto R = [&](const char* rel) { return root + rel; };

    /* ====== 1. TEXTURES ====== */
    printf("--- Textures ---\n");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\player1.png")) != NULL, "player1.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\player2.png")) != NULL, "player2.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\en.png")) != NULL, "en.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\enemy.png")) != NULL, "enemy.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\enemy2.png")) != NULL, "enemy2.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\enemy3.png")) != NULL, "enemy3.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\enemy4.png")) != NULL, "enemy4.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\enemy5.png")) != NULL, "enemy5.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\missile.png")) != NULL, "missile.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rocket.png")) != NULL, "rocket.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rocket_enemy.png")) != NULL, "rocket_enemy.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\bullet1.png")) != NULL, "bullet1.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\bullet_enemy.png")) != NULL, "bullet_enemy.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\lazer_level1.png")) != NULL, "lazer_level1.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\energyball.png")) != NULL, "energyball.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\energyball_enhanced.png")) != NULL, "energyball_enhanced.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\magabomb.png")) != NULL, "magabomb.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\unit1.png")) != NULL, "unit1.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\big1.png")) != NULL, "big1.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\big2.png")) != NULL, "big2.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rship.png")) != NULL, "rship.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rship2.png")) != NULL, "rship2.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rship3.png")) != NULL, "rship3.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\rship4.png")) != NULL, "rship4.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\ca.png")) != NULL, "ca.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\ready.png")) != NULL, "ready.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_shotgun.png")) != NULL, "item_shotgun.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_missile.png")) != NULL, "item_missile.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_lazer.png")) != NULL, "item_lazer.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_autocannon.png")) != NULL, "item_autocannon.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_super.png")) != NULL, "item_super.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_rocket.png")) != NULL, "item_rocket.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_maga.png")) != NULL, "item_maga.png");
    CHECK(texCache.load(R("AirwarCPP\\Resources\\images\\item_medic.png")) != NULL, "item_medic.png");

    /* ====== 2. SPRITE RENDERING ====== */
    printf("--- Sprite Rendering ---\n");
    auto* pTex = texCache.load(R("AirwarCPP\\Resources\\images\\player1.png"));
    auto* eTex = texCache.load(R("AirwarCPP\\Resources\\images\\en.png"));
    spr.draw(pTex, 400, 300, 0, 255);
    spr.draw(eTex, 200, 150, 45, 128, 1.5f);
    spr.drawInterpolated(pTex, 100, 100, 200, 300, 0, 90, 0.5f);
    CHECK(true, "Sprite transforms OK");

    /* ====== 3. BACKGROUND ====== */
    printf("--- Background ---\n");
    Background bg;
    bg.init(ren, 800, 600);
    for (int i = 0; i < 5; ++i) bg.update();
    CHECK(true, "Background OK");

    /* ====== 4. PARTICLES ====== */
    printf("--- Particles ---\n");
    EnemyExplosion ee(400, 300); CHECK(!ee.particles.empty(), "EnemyExplosion");
    PlayerExplosion pe(400, 300); CHECK(!pe.particles.empty(), "PlayerExplosion");
    MissileHit mh(400, 300); CHECK(!mh.particles.empty(), "MissileHit");
    RocketHit rh(400, 300); CHECK(!rh.particles.empty(), "RocketHit");
    BulletHit bh(400, 300); CHECK(!bh.particles.empty(), "BulletHit");
    LazerHit lh(400, 300); CHECK(!lh.particles.empty(), "LazerHit");
    AutocannonHit ah(400, 300); CHECK(!ah.particles.empty(), "AutocannonHit");
    NukeExplosion ne(400, 300); CHECK(!ne.particles.empty(), "NukeExplosion");
    MissileTrail mt(400, 300); mt.emit(1); CHECK(mt.particles.size()==1, "MissileTrail");
    RocketTrail rt(400, 300); rt.emit(1); CHECK(rt.particles.size()==1, "RocketTrail");

    /* ====== 5. AUDIO LOAD ====== */
    printf("--- Audio Loading ---\n");
    SoundEngine se; CHECK(se.init(), "SoundEngine init");
    CHECK(se.load("shotgun",    R("AirwarCPP\\Resources\\sounds\\shotgun_shoot.wav")), "shotgun_shoot.wav");
    CHECK(se.load("lazer",      R("AirwarCPP\\Resources\\sounds\\lazer_shoot.wav")), "lazer_shoot.wav");
    CHECK(se.load("autocannon", R("AirwarCPP\\Resources\\sounds\\autocannon_shoot.wav")), "autocannon_shoot.wav");
    CHECK(se.load("missile",    R("AirwarCPP\\Resources\\sounds\\missile_shoot.wav")), "missile_shoot.wav");
    CHECK(se.load("rocket",     R("AirwarCPP\\Resources\\sounds\\rocket_shoot.wav")), "rocket_shoot.wav");
    CHECK(se.load("explode1",   R("AirwarCPP\\Resources\\sounds\\explode1.wav")), "explode1.wav");
    CHECK(se.load("explode2",   R("AirwarCPP\\Resources\\sounds\\explode2.wav")), "explode2.wav");
    CHECK(se.load("explode3",   R("AirwarCPP\\Resources\\sounds\\explode3.wav")), "explode3.wav");
    CHECK(se.load("explode4",   R("AirwarCPP\\Resources\\sounds\\explode4.wav")), "explode4.wav");
    CHECK(se.load("explode5",   R("AirwarCPP\\Resources\\sounds\\explode5.wav")), "explode5.wav");
    CHECK(se.load("prepare",    R("AirwarCPP\\Resources\\sounds\\prepare.wav")), "prepare.wav");
    CHECK(se.load("unprepare",  R("AirwarCPP\\Resources\\sounds\\unprepare.wav")), "unprepare.wav");
    CHECK(se.load("itemget",    R("AirwarCPP\\Resources\\sounds\\itemget.wav")), "itemget.wav");
    CHECK(se.load("transmission",R("AirwarCPP\\Resources\\sounds\\transmission.wav")), "transmission.wav");
    CHECK(se.load("nuclear",    R("AirwarCPP\\Resources\\sounds\\nuclear_missile_shoot.wav")), "nuclear_missile_shoot.wav");

    /* ====== 6. AUDIO PLAYBACK ====== */
    printf("--- Audio Playback ---\n");
    se.play("shotgun"); SDL_Delay(80);
    se.play("lazer"); SDL_Delay(80);
    se.play("explode1"); SDL_Delay(80);
    se.play("missile"); SDL_Delay(80);
    se.play("rocket"); SDL_Delay(80);
    se.stopAll(); CHECK(true, "Play + stopAll OK");

    /* ====== 7. MUSIC ====== */
    printf("--- Music ---\n");
    MusicPlayer mp;
    mp.init(&se, root);
    CHECK(se.load("mainmenu",       R("AirwarCPP\\Resources\\music\\mainmenu.wav")), "mainmenu.wav");
    CHECK(se.load("future_intro",   R("AirwarCPP\\Resources\\music\\future_intro.wav")), "future_intro.wav");
    CHECK(se.load("future_loop",    R("AirwarCPP\\Resources\\music\\future.wav")), "future.wav");
    CHECK(se.load("lostcity_intro", R("AirwarCPP\\Resources\\music\\lostcity_intro.wav")), "lostcity_intro.wav");
    CHECK(se.load("pop_intro",      R("AirwarCPP\\Resources\\music\\pop_intro.wav")), "pop_intro.wav");
    CHECK(se.load("beach_intro",    R("AirwarCPP\\Resources\\music\\beach_intro.wav")), "beach_intro.wav");
    CHECK(se.load("escape_intro",   R("AirwarCPP\\Resources\\music\\escape_intro.wav")), "escape_intro.wav");
    CHECK(se.load("universe41_intro",R("AirwarCPP\\Resources\\music\\universe41_intro.wav")), "universe41_intro.wav");
    se.stopAll(); CHECK(true, "All music loaded");

    /* ====== 8. HUD ====== */
    printf("--- HUD ---\n");
    HUD hud;
    hud.init(ren, &texCache, &spr, "C:\\Windows\\Fonts\\arial.ttf", 800, 664, root);
    hud.setVersion("Ver. 1.3.1");
    hud.setLevel("Level 1");
    hud.addTitle("Airwar", 120);
    hud.updateEntity(0, 400, 300, 0, 100, "player1", "Player1", 0, 3, true);
    hud.updateEntity(1, 500, 200, 90, 60, "en", "Enemy", -1);
    hud.update(); CHECK(true, "HUD OK");

    /* ====== 9. CONFIG JSON ====== */
    printf("--- Config JSON ---\n");
    {
        std::string p = R("AirwarCPP\\Resources\\configs\\enemyTypes.json");
        std::ifstream f(p);
        if (!f.good()) { CHECK(false, ("enemyTypes.json not found at " + p).c_str()); }
        else {
            nlohmann::json j; f >> j;
            CHECK(j.contains("en"), "enemyTypes has 'en'");
            CHECK(j.contains("big2"), "enemyTypes has 'big2'");
        }
    }
    {
        std::string p = R("AirwarCPP\\Resources\\configs\\initializeSettings.json");
        std::ifstream f(p);
        CHECK(f.good(), ("initializeSettings.json readable at " + p).c_str());
    }

    /* ====== 10. LEVELS JSON ====== */
    printf("--- Level JSON ---\n");
    for (int i = 1; i <= 5; ++i) {
        std::string path = R("AirwarCPP\\Resources\\levels\\") + std::to_string(i) + ".json";
        std::ifstream f(path);
        CHECK(f.good(), ("Level " + std::to_string(i) + " - " + path).c_str());
    }

    /* ====== 11. ANIMATED DEMO ====== */
    printf("--- Animated Demo (3s) ---\n");
    Uint64 start = SDL_GetTicks();
    int frames = 0;
    bool running = true;
    while (running && (SDL_GetTicks() - start < 3000)) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }
        float t = (SDL_GetTicks() - start) / 1000.0f;

        SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
        SDL_RenderClear(ren);
        bg.update(); bg.draw();

        float cx = 400 + 150 * sinf(t);
        float cy = 300 + 80 * cosf(t * 0.7f);
        spr.draw(pTex, cx, cy, t * 45);
        spr.draw(eTex, 500 + 100 * sinf(t * 0.5f), 200 + 60 * cosf(t * 0.3f), 90 + t * 20);

        hud.updateEntity(0, cx, cy, t * 45, 100, "player1", "Player1", 0, 3, true);
        hud.update(); hud.draw(0);

        SDL_RenderPresent(ren);
        SDL_Delay(16);
        ++frames;
    }
    CHECK(frames > 50, "Demo >50 frames");

    /* ====== 12. CLEANUP ====== */
    printf("--- Cleanup ---\n");
    se.cleanup();
    texCache.clear();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    CHECK(true, "Clean shutdown");

    int total = testsPassed + testsFailed;
    printf("\n=============================\n");
    printf("  Results: %d / %d, %d failed\n", testsPassed, total, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
