#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#undef min
#undef max
#endif
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "../Core/RNG.h"
#include "../Core/Constants.h"
#include "../Core/Message.h"
#include "../Core/Queue.h"
#include "../Game/Game.h"
#include "../Game/Board.h"
#include "../Render/TextureCache.h"
#include "../Render/SpriteRenderer.h"
#include "../Render/Background.h"
#include "../Render/Particle.h"
#include "../UI/HUD.h"
#include "../Net/UdpClient.h"
#include "../Net/GameClient.h"

// Returns normalized project root path (navigates up from exe dir)
static std::string getProjectRoot() {
    const char* base = SDL_GetBasePath();
    if (!base) return "";
    std::string p = base;
    SDL_free(const_cast<char*>(base));
    // exe is at: out/build/x64-debug/AirwarCPP/Airwar.exe
    // repo root: ../../../../ (4 levels up from build dir)
    p += "..\\..\\..\\..\\";
    // Normalize: resolve .. segments
    char full[MAX_PATH];
    GetFullPathNameA(p.c_str(), MAX_PATH, full, NULL);
    p = full;
    if (!p.empty() && p.back() != '\\') p += '\\';
    return p;
}

// Interpolation state for smooth rendering
struct EntityRenderState {
    float prevX = 0, prevY = 0, prevRot = 0;
    float currX = 0, currY = 0, currRot = 0;
    float health = 100;
    std::string image;
    bool isReady = false;
    int playerId = -1;
    std::string name;
    int magabombQty = 0;
    Uint64 lastUpdate = 0;
};

int main(int, char**) {
#ifdef _WIN32
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

    std::string root = getProjectRoot();
    seedRNG();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        printf("SDL_Init failed: %s\n", SDL_GetError()); return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Airwar", 800, 664, 0);
    if (!win) return 1;
    SDL_Renderer* ren = SDL_CreateRenderer(win, NULL);
    if (!ren) return 1;

    TextureCache texCache;
    texCache.init(ren);
    SpriteRenderer spr;
    spr.init(ren);
    Background bg;
    bg.init(ren, 800, 600);

    // Font and HUD
    std::string fontPath = "C:\\Windows\\Fonts\\arial.ttf";
    HUD hud;
    hud.init(ren, &texCache, &spr, fontPath, 800, 664, root);

    // Game setup
    Queue<Message> msgQueue;
    auto game = std::make_shared<Game>(msgQueue);
    SinglePlayerClient client(0, game, msgQueue, "Player1");
    client.newPlayer();

    // Mark player as ready
    if (!game->board.players.empty()) {
        game->board.players[0]->isReady = true;
    }

    // Preload textures
    texCache.load(root + "AirwarCPP\\Resources\\images\\player1.png");
    texCache.load(root + "AirwarCPP\\Resources\\images\\en.png");
    texCache.load(root + "AirwarCPP\\Resources\\images\\missile.png");
    texCache.load(root + "AirwarCPP\\Resources\\images\\bullet1.png");

    // Particle groups
    std::vector<std::unique_ptr<ParticleGroup>> particleGroups;
    std::unordered_map<int, std::unique_ptr<ParticleGroup>> trails;

    // Title
    hud.setVersion("Ver. 1.3.1");
    hud.addTitle("Airwar", 180);

    // Game loop
    const double GAME_TICK_MS = 1000.0 / GAME_TICK;
    double tickAccumulator = 0.0;
    Uint64 lastTime = SDL_GetTicks();
    bool running = true;

    while (running) {
        Uint64 now = SDL_GetTicks();
        double dt = (double)(now - lastTime);
        lastTime = now;
        tickAccumulator += dt;

        // ── Input ──
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) running = false;
                // Pause via P
                if (e.key.key == Keys::p && !game->board.players.empty()) {
                    if (game->isPaused && game->pausePlayerId == 0) {
                        game->isPaused = false;
                        game->pausePlayerId = -1;
                        game->pausePlayerName = "";
                    } else if (!game->isPaused) {
                        game->isPaused = true;
                        game->pausePlayerId = 0;
                        game->pausePlayerName = game->board.players[0]->name;
                    }
                }
                client.sendMessage(Message("0", "keyDown", {{"key", (int)e.key.key}}));
            }
            if (e.type == SDL_EVENT_KEY_UP) {
                client.sendMessage(Message("0", "keyUp", {{"key", (int)e.key.key}}));
            }
        }

        // ── Game tick (30 Hz) ──
        while (tickAccumulator >= GAME_TICK_MS) {
            tickAccumulator -= GAME_TICK_MS;

            // Check if game should progress from mainMenu → inGame
            if (game->currState() == GameState::mainMenu) {
                game->setCurrState(GameState::loadLevel);
                hud.addTitle("Wave 1", 90);
                game->setCurrState(GameState::inGame);
                hud.addTitle("Wave 1", 90);

                // Spawn initial enemies
                for (int i = 0; i < 5; ++i) {
                    auto enemy = std::make_shared<Enemy>();
                    enemy->x = 100 + (rand() % 600);
                    enemy->y = -50 - (rand() % 100);
                    enemy->image = Images::en;
                    enemy->health = 50 + (rand() % 100);
                    enemy->boundingBox = std::make_shared<BoundingBox>(40, 40);
                    enemy->race = Race::enemy;
                    game->board.addUnit(enemy, "unit");
                }
                hud.addTitle("Go!", 60);
            }

            if (!game->isPaused) {
                // Apply player input to movement
                if (!game->board.players.empty()) {
                    auto& p = game->board.players[0];
                    double speed = 5.0;
                    double vx = 0, vy = 0;
                    for (int k : p->pressedKeyList) {
                        if (k == Keys::w) vy = -speed;
                        if (k == Keys::s) vy = speed;
                        if (k == Keys::a) vx = -speed;
                        if (k == Keys::d) vx = speed;
                    }
                    p->velocity = Vector(vx, vy);

                    // Auto-shoot
                    p->weapon->setShooting(true);
                }

                client.update();

                // Respawn enemies if all dead
                bool hasEnemy = false;
                for (auto& u : game->board.units) {
                    if (u->race == Race::enemy && u->isAlive) {
                        hasEnemy = true; break;
                    }
                }
                if (!hasEnemy) {
                    for (int i = 0; i < 5 + (rand() % 3); ++i) {
                        auto enemy = std::make_shared<Enemy>();
                        enemy->x = 50 + (rand() % 700);
                        enemy->y = -50 - (rand() % 100);
                        enemy->image = Images::en;
                        enemy->health = 60 + (rand() % 140);
                        enemy->boundingBox = std::make_shared<BoundingBox>(40, 40);
                        enemy->race = Race::enemy;
                        game->board.addUnit(enemy, "unit");
                    }
                }

                // Process sound/particle messages from msgQueue
                Message m;
                while (msgQueue.tryPop(m)) {
                    if (m.type == "particle_effect") {
                        auto& c = m.content;
                        std::string effect = c["effect"].get<std::string>();
                        float px = c["x"].get<float>();
                        float py = c["y"].get<float>();
                        if (effect == "enemy_explosion")
                            particleGroups.push_back(std::make_unique<EnemyExplosion>(px, py));
                        else if (effect == "player_explosion")
                            particleGroups.push_back(std::make_unique<PlayerExplosion>(px, py));
                        else if (effect == "missile_hit")
                            particleGroups.push_back(std::make_unique<MissileHit>(px, py));
                        else if (effect == "bullet_hit")
                            particleGroups.push_back(std::make_unique<BulletHit>(px, py));
                        else if (effect == "nuke_explosion")
                            particleGroups.push_back(std::make_unique<NukeExplosion>(px, py));
                    }
                }
            }
        }

        // ── Render (60 FPS) ──
        float alpha = (float)(tickAccumulator / GAME_TICK_MS); // interpolation factor

        SDL_SetRenderDrawColor(ren, 35, 90, 150, 255);
        SDL_RenderClear(ren);
        bg.update();
        bg.draw();

        // Build entity render states from board
        hud.clearEntities();
        for (auto& player : game->board.players) {
            hud.updateEntity(player->id, (float)player->x, (float)player->y,
                (float)player->rotation, (float)player->health,
                ImageNames.at(player->image.value_or(Images::player1)),
                player->name, player->player_id, player->magabombQuantity, player->isReady);
        }
        for (auto& unit : game->board.units) {
            auto* e = unit.get();
            std::string img = "en";
            if (e->image.has_value()) img = ImageNames.at(e->image.value());
            float hp = 100;
            if (auto u = dynamic_cast<Unit*>(e)) hp = (float)u->health;
            hud.updateEntity(e->id, (float)e->x, (float)e->y, (float)e->rotation, hp, img);
        }
        for (auto& proj : game->board.projectiles) {
            auto* e = proj.get();
            std::string img = "bullet1";
            if (e->image.has_value()) img = ImageNames.at(e->image.value());
            hud.updateEntity(e->id, (float)e->x, (float)e->y, (float)e->rotation, 0, img);
        }

        hud.update();
        hud.draw(0);

        // Direct sprite rendering (fallback independent of HUD)
        auto drawSprite = [&](float x, float y, float rot, const std::string& texFile) {
            auto* t = texCache.load(root + "AirwarCPP\\Resources\\images\\" + texFile);
            if (t) spr.draw(t, x, y, rot);
        };
        for (auto& p : game->board.players)
            drawSprite((float)p->x, (float)p->y, (float)p->rotation, "player1.png");
        for (auto& u : game->board.units) {
            std::string f = "en.png";
            if (u->image.has_value()) {
                auto it = ImageNames.find(u->image.value());
                if (it != ImageNames.end()) f = it->second + ".png";
            }
            drawSprite((float)u->x, (float)u->y, (float)u->rotation, f);
        }
        for (auto& pj : game->board.projectiles) {
            std::string f = "bullet1.png";
            if (pj->image.has_value()) {
                auto it = ImageNames.find(pj->image.value());
                if (it != ImageNames.end()) f = it->second + ".png";
            }
            drawSprite((float)pj->x, (float)pj->y, (float)pj->rotation, f);
        }

        // Update and draw particles
        for (auto it = particleGroups.begin(); it != particleGroups.end(); ) {
            (*it)->update();
            (*it)->draw(ren);
            if ((*it)->particleCount() == 0) it = particleGroups.erase(it); else ++it;
        }

        // Projectile trails
        for (auto& proj : game->board.projectiles) {
            if (proj->image == Images::missile) {
                if (trails.find(proj->id) == trails.end()) {
                    trails[proj->id] = std::make_unique<MissileTrail>((float)proj->x, (float)proj->y);
                }
                auto& trail = trails[proj->id];
                trail->x = (float)proj->x;
                trail->y = (float)proj->y;
                if (trail->particleCount() < trail->maxParticles && rand() % 3 == 0) {
                    trail->config = {{10, 20}, {2, 0.5f}, {180, 80}, {180, 80}, {180, 80}, {200, 0}, 0, 0};
                    trail->emit(1);
                }
            }
        }
        for (auto& [id, trail] : trails) {
            trail->update();
            trail->draw(ren);
        }
        // Clean dead trails
        for (auto it = trails.begin(); it != trails.end(); ) {
            bool alive = false;
            for (auto& proj : game->board.projectiles)
                if (proj->id == it->first) { alive = true; break; }
            if (!alive) it = trails.erase(it); else ++it;
        }

        SDL_RenderPresent(ren);

        // Frame rate cap
        Uint64 frameTime = SDL_GetTicks() - now;
        if (frameTime < 16) SDL_Delay(16 - (Uint32)frameTime);
    }

    texCache.clear();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    return 0;
}
