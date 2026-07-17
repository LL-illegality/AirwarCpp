#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <unordered_map>

struct TitleInfo {
    std::string text;
    int duration = 0;
    int delay = 0;
};

class HUD {
    SDL_Renderer* renderer_ = nullptr;
    int width_ = 800, height_ = 664;
public:
    std::vector<TitleInfo> titles_;  // exposed for testing
private:
    std::string versionText_ = "Ver. 1.3.1";
    std::string levelInfo_;

    // Entity snapshot for interpolation
    struct EntityData {
        float prevX = 0, prevY = 0, prevRot = 0;
        float currX = 0, currY = 0, currRot = 0;
        float health = 100, alpha = 255;
        std::string image;
        std::string name;
        int playerId = -1;
        int magabombQty = 0;
        bool isReady = false;
        Uint64 lastUpdate = 0;
    };
    std::unordered_map<int, EntityData> entities_;

public:
    void init(SDL_Renderer* r, int w, int h) { renderer_ = r; width_ = w; height_ = h; }

    void setLevelInfo(const std::string& info) { levelInfo_ = info; }
    void setTitle(const std::string& text, int duration) {
        titles_.push_back({text, duration, duration});
    }
    void setVersion(const std::string& v) { versionText_ = v; }

    void updateEntity(int id, float x, float y, float rot, float health,
                      const std::string& image, const std::string& name = "",
                      int playerId = -1, int magabombQty = 0, bool isReady = false,
                      float alpha = 255) {
        auto& e = entities_[id];
        e.prevX = e.currX; e.prevY = e.currY; e.prevRot = e.currRot;
        e.currX = x; e.currY = y; e.currRot = rot;
        e.health = health; e.image = image; e.name = name;
        e.playerId = playerId; e.magabombQty = magabombQty;
        e.isReady = isReady; e.alpha = alpha;
        e.lastUpdate = SDL_GetTicks();
    }

    void clearEntities() { entities_.clear(); }

    void update() {
        // Fade titles
        for (auto it = titles_.begin(); it != titles_.end(); ) {
            it->delay -= 1;
            if (it->delay <= 0) it = titles_.erase(it); else ++it;
        }
    }

    void draw(int localPlayerId = -1) {
        if (!renderer_) return;

        // Draw entities with interpolation
        Uint64 now = SDL_GetTicks();
        for (auto& [id, e] : entities_) {
            float t = (now - e.lastUpdate) / 33.0f;  // 33ms = 1 game tick
            if (t > 1.0f) t = 1.0f;

            float drawX = e.prevX + (e.currX - e.prevX) * t;
            float drawY = e.prevY + (e.currY - e.prevY) * t;
            float drawRot = e.prevRot + (e.currRot - e.prevRot) * t;

            // Draw player name above entity
            if (!e.name.empty()) {
                // Name text would use TTF — for now skip text rendering
                // and just mark position with a colored line
                SDL_SetRenderDrawColor(renderer_, 19, 19, 19, 255);
                SDL_RenderLine(renderer_, drawX - 10, drawY + 30, drawX + 10, drawY + 30);
            }

            // Draw ready indicator
            if (e.isReady && e.playerId >= 0) {
                SDL_SetRenderDrawColor(renderer_, 0, 192, 0, 255);
                SDL_FRect r = { drawX - 6, drawY - 48, 12, 12 };
                SDL_RenderFillRect(renderer_, &r);
            }

            // Draw local player's status bar
            if (e.playerId == localPlayerId) {
                drawStateBar(e.name, e.health, e.magabombQty);
            }
        }

        // Draw titles
        for (auto& t : titles_) {
            float fade = (t.delay <= GAME_TICK) ? (float)t.delay / GAME_TICK : 1.0f;
            if (fade <= 0) continue;
            int alpha = (int)(fade * 255);
            SDL_SetRenderDrawColor(renderer_, 19, 19, 19, alpha);
            // Title text would use TTF — for now draw a simple rectangle marker
            SDL_FRect r = { (float)(width_ / 2 - 100), (float)(height_ / 4 - 10), 200, 20 };
            SDL_RenderFillRect(renderer_, &r);
        }

        // Draw level info
        if (titles_.empty() && !levelInfo_.empty()) {
            SDL_SetRenderDrawColor(renderer_, 19, 19, 19, 255);
            SDL_FRect r = { (float)(width_ / 2 - 50), 8, 100, 16 };
            SDL_RenderFillRect(renderer_, &r);
        }

        // Draw version in top-right corner
        SDL_SetRenderDrawColor(renderer_, 19, 19, 19, 128);
        SDL_FRect verRect = { (float)(width_ - 100), 4, 96, 12 };
        SDL_RenderFillRect(renderer_, &verRect);
    }

private:
    void drawStateBar(const std::string& name, float health, int magabombQty) {
        const int barY = height_ - 64;
        const int barH = 64;

        // Background
        SDL_SetRenderDrawColor(renderer_, 192, 192, 192, 255);
        SDL_FRect bg = { 0, (float)barY, (float)width_, (float)barH };
        SDL_RenderFillRect(renderer_, &bg);

        // HP bar border
        SDL_SetRenderDrawColor(renderer_, 19, 19, 19, 255);
        SDL_FRect hpBorder = { 35, (float)(barY + 25), 202, 18 };
        SDL_RenderFillRect(renderer_, &hpBorder);

        // HP bar fill
        float hpRatio = std::max(0.0f, std::min(1.0f, health / 100.0f));
        Uint8 hpR = (Uint8)(255 * (1 - hpRatio));
        Uint8 hpG = (Uint8)(255 * hpRatio);
        SDL_SetRenderDrawColor(renderer_, hpR, hpG, 0, 255);
        SDL_FRect hpFill = { 36, (float)(barY + 26), (float)(std::max(0.0, (double)(hpRatio * 200))), 16 };
        if (hpFill.w > 0) SDL_RenderFillRect(renderer_, &hpFill);

        // Magabomb icons
        for (int i = 0; i < magabombQty && i < 10; ++i) {
            SDL_SetRenderDrawColor(renderer_, 200, 50, 50, 255);
            SDL_FRect bomb = { (float)(width_ - 40 - i * 32), (float)(barY + 28), 8, 8 };
            SDL_RenderFillRect(renderer_, &bomb);
        }

        // Draw marker triangle
        SDL_SetRenderDrawColor(renderer_, 0, 192, 0, 255);
    }
};
