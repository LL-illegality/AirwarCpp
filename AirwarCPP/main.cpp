#include <cstdio>
#include <cstdlib>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "json.hpp"

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST(condition, msg)                                                \
    do {                                                                    \
        if (condition) {                                                    \
            ++testsPassed;                                                  \
            printf("  PASS: %s\n", msg);                                   \
        } else {                                                            \
            ++testsFailed;                                                  \
            printf("  FAIL: %s\n", msg);                                   \
        }                                                                   \
    } while (0)

#define CHECK_SDL(condition, msg)                                           \
    do {                                                                    \
        if (!(condition)) {                                                 \
            printf("  FAIL: %s\n  Error: %s\n",                            \
                   msg, SDL_GetError());                                    \
            ++testsFailed;                                                  \
        } else {                                                            \
            ++testsPassed;                                                  \
            printf("  PASS: %s\n", msg);                                   \
        }                                                                   \
    } while (0)

static std::string assetRoot;

static bool initAssetRoot() {
    const char* base = SDL_GetBasePath();
    if (!base) return false;
    assetRoot = base;
    SDL_free(const_cast<char*>(base));
    assetRoot += "..\\..\\..\\..\\";

    return true;
}

static std::string getAssetPath(const std::string& relative) {
    return assetRoot + relative;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("AirwarCPP Phase 0 -- Build & Validation Test\n");
    printf("============================================\n\n");

    /* ---- JSON (nlohmann) ---- */
    printf("[Step 1] nlohmann/json header\n");
    nlohmann::json j = {{"name", "AirwarCPP"}, {"version", 0}, {"phase", "Phase0"}};
    TEST(j["name"] == "AirwarCPP" && j["version"] == 0, "nlohmann/json construct & access");

    /* ---- SDL_Init ---- */
    printf("[Step 2] SDL_Init\n");
    CHECK_SDL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), "SDL_Init(VIDEO|AUDIO)");
    printf("[Step 2b] Asset root\n");
    TEST(initAssetRoot(), "initAssetRoot()");

    /* TTF */
    printf("[Step 3] SDL3_ttf\n");
    CHECK_SDL(TTF_Init(), "TTF_Init()");

    /* MIX */
    printf("[Step 4] SDL3_mixer\n");
    CHECK_SDL(MIX_Init(), "MIX_Init()");

    /* IMG - just test it links */
    printf("[Step 5] SDL3_image\n");
    SDL_Surface* s = IMG_Load("nonexistent.png");
    if (!s) { printf("  (IMG_Load linked OK, expected fail on bad file)\n"); }
    else { SDL_DestroySurface(s); }
    TEST(true, "SDL3_image linked");

    /* Window + Renderer */
    printf("[Step 6] Window & Renderer\n");
    SDL_Window* window = SDL_CreateWindow("AirwarCPP Phase 0", 800, 600, 0);
    CHECK_SDL(window != NULL, "SDL_CreateWindow(800x600)");
    if (!window) return 1;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    CHECK_SDL(renderer != NULL, "SDL_CreateRenderer");
    if (!renderer) return 1;

    /* PNG texture */
    printf("[Step 7] PNG Texture\n");
    std::string pngPath = getAssetPath("Airwar_python\\images\\player1.png");
    printf("       path = %s\n", pngPath.c_str());
    SDL_Surface* surf = IMG_Load(pngPath.c_str());
    CHECK_SDL(surf != NULL, "IMG_Load(player1.png)");
    if (!surf) return 1;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    CHECK_SDL(tex != NULL, "SDL_CreateTextureFromSurface");
    SDL_DestroySurface(surf);

    float tw = 0, th = 0;
    SDL_GetTextureSize(tex, &tw, &th);
    printf("       texture size = %.0f x %.0f\n", tw, th);
    TEST(tw > 0 && th > 0, "valid texture size");

    /* WAV */
    printf("[Step 8] WAV Playback\n");
    MIX_Mixer* mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    CHECK_SDL(mixer != NULL, "MIX_CreateMixerDevice");
    if (!mixer) return 1;

    std::string wavPath = getAssetPath("Airwar_python\\sounds\\explode1.wav");
    printf("       path = %s\n", wavPath.c_str());
    MIX_Audio* audio = MIX_LoadAudio(mixer, wavPath.c_str(), true);
    CHECK_SDL(audio != NULL, "MIX_LoadAudio(explode1.wav)");

    MIX_Track* track = NULL;
    if (audio) {
        track = MIX_CreateTrack(mixer);
        CHECK_SDL(track != NULL, "MIX_CreateTrack");
        if (track) {
            MIX_SetTrackAudio(track, audio);
            CHECK_SDL(MIX_PlayTrack(track, 0), "MIX_PlayTrack");
            printf("       -> playing ...\n");
        }
    }

    /* TTF */
    printf("[Step 9] TTF Text\n");
    TTF_Font* font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 40.0f);
    CHECK_SDL(font != NULL, "TTF_OpenFont");

    SDL_Texture* textTex = NULL;
    float textW = 0, textH = 0;
    if (font) {
        SDL_Color white = { 255, 255, 255, 255 };
        const char* msg = "AirwarCPP Phase 0 PASS";
        SDL_Surface* ts = TTF_RenderText_Blended(font, msg, SDL_strlen(msg), white);
        CHECK_SDL(ts != NULL, "TTF_RenderText_Blended");
        if (ts) {
            textTex = SDL_CreateTextureFromSurface(renderer, ts);
            CHECK_SDL(textTex != NULL, "SDL_CreateTextureFromSurface(text)");
            SDL_GetTextureSize(textTex, &textW, &textH);
            SDL_DestroySurface(ts);
        }
    }

    /* JSON round-trip */
    printf("[Step 10] JSON round-trip\n");
    {
        nlohmann::json orig = {
            {"objects", nlohmann::json::array()},
            {"isPaused", false},
            {"pausePlayerName", ""}
        };
        orig["objects"].push_back({
            {"id", 0}, {"x", 400.0}, {"y", 300.0}, {"rotation", 0.0},
            {"image", "player1"}, {"health", 100.0}
        });
        std::string serialized = orig.dump();
        nlohmann::json parsed = nlohmann::json::parse(serialized);
        bool ok = parsed["objects"][0]["id"] == 0 &&
                  parsed["objects"][0]["x"] == 400.0 &&
                  parsed["isPaused"] == false;
        TEST(ok, "ScreenInfo round-trip");
    }

    /* Render loop (3 seconds) */
    printf("\n----- Entering render loop for 3 seconds -----\n");
    bool running = true;
    Uint64 start = SDL_GetTicks();
    SDL_FRect dst = { (800.0f - tw) / 2.0f, (600.0f - th) / 2.0f - 40.0f, tw, th };
    SDL_FRect tdst = { (800.0f - textW) / 2.0f, (600.0f - th) / 2.0f + 40.0f, textW, textH };

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_ESCAPE) running = false;
        }
        if (SDL_GetTicks() - start >= 3000) running = false;

        SDL_SetRenderDrawColor(renderer, 35, 90, 150, 255);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, tex, NULL, &dst);
        if (textTex) SDL_RenderTexture(renderer, textTex, NULL, &tdst);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    /* Cleanup: destroy all objects, then quit subsystems in reverse init order */
    if (track)  MIX_StopTrack(track, 0);
    if (font)   TTF_CloseFont(font);
    if (textTex) SDL_DestroyTexture(textTex);
    if (tex)    SDL_DestroyTexture(tex);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    MIX_Quit();
    TTF_Quit();
    SDL_Quit();

    printf("\n============================================\n");
    printf("  Results: %d passed, %d failed\n", testsPassed, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
