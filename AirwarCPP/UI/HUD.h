#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "../Render/TextureCache.h"
#include "../Render/SpriteRenderer.h"
#include <string>
#include <vector>
#include <unordered_map>

struct TitleInfo {
    std::string text;
    int duration = 0, delay = 0;
};

class HUD {
    SDL_Renderer* renderer_ = nullptr;
    TextureCache* texCache_ = nullptr;
    SpriteRenderer* spriteRenderer_ = nullptr;
    TTF_Font* fontSmall_ = nullptr;
    TTF_Font* fontLarge_ = nullptr;
    int width_ = 800, height_ = 664;
    std::string version_ = "Ver. 1.3.1";
    std::string levelInfo_;
    std::vector<TitleInfo> titles_;
    bool fontReady_ = false;

    struct EntityVis {
        float prevX=0, prevY=0, prevRot=0, currX=0, currY=0, currRot=0;
        float health=100;
        std::string image;
        std::string name;
        int playerId=-1, magabombQty=0;
        bool isReady=false;
        Uint64 lastUpdate=0;
    };
    std::unordered_map<int, EntityVis> entities_;

public:
    ~HUD() {
        if (fontSmall_) TTF_CloseFont(fontSmall_);
        if (fontLarge_) TTF_CloseFont(fontLarge_);
    }

    void init(SDL_Renderer* r, TextureCache* tc, SpriteRenderer* sr,
              const std::string& fontPath, int w, int h) {
        renderer_ = r; texCache_ = tc; spriteRenderer_ = sr;
        width_ = w; height_ = h;
        fontSmall_ = TTF_OpenFont(fontPath.c_str(), 18.0f);
        fontLarge_ = TTF_OpenFont(fontPath.c_str(), 28.0f);
        fontReady_ = (fontSmall_ != nullptr);
        if (!fontReady_) SDL_Log("TTF_OpenFont(%s) failed: %s", fontPath.c_str(), SDL_GetError());
    }

    void setLevel(const std::string& lvl) { levelInfo_ = lvl; }
    void setVersion(const std::string& v) { version_ = v; }
    void addTitle(const std::string& text, int dur) { titles_.push_back({text, dur, dur}); }

    void updateEntity(int id, float x, float y, float rot, float hp,
                      const std::string& texName, const std::string& name="",
                      int pid=-1, int bombs=0, bool ready=false) {
        auto& e = entities_[id];
        e.prevX = e.currX; e.prevY = e.currY; e.prevRot = e.currRot;
        e.currX = x; e.currY = y; e.currRot = rot;
        e.health = hp; e.image = texName; e.name = name;
        e.playerId = pid; e.magabombQty = bombs; e.isReady = ready;
        e.lastUpdate = SDL_GetTicks();
    }

    void clearEntities() { entities_.clear(); }

    void update() {
        for (auto it = titles_.begin(); it != titles_.end(); ) {
            --it->delay;
            if (it->delay <= 0) it = titles_.erase(it); else ++it;
        }
    }

    void draw(int localPlayerId = -1) {
        if (!renderer_ || !spriteRenderer_) return;
        Uint64 now = SDL_GetTicks();

        for (auto& [id, e] : entities_) {
            float t = std::min(1.0f, (now - e.lastUpdate) / 33.0f);
            float dx = e.prevX + (e.currX - e.prevX) * t;
            float dy = e.prevY + (e.currY - e.prevY) * t;
            float dr = e.prevRot + (e.currRot - e.prevRot) * t;

            // Draw entity sprite from texture cache
            auto* tex = texCache_ ? texCache_->load(
                "AirwarCPP\\Resources\\images\\" + e.image + ".png") : nullptr;
            if (tex) spriteRenderer_->draw(tex, dx, dy, dr);

            // Draw player name below sprite
            if (!e.name.empty() && fontReady_ && fontSmall_) {
                auto* surf = TTF_RenderText_Blended(fontSmall_, e.name.c_str(),
                    e.name.size(), {19,19,19,255});
                if (surf) {
                    auto* nameTex = SDL_CreateTextureFromSurface(renderer_, surf);
                    SDL_DestroySurface(surf);
                    if (nameTex) {
                        float nw, nh;
                        SDL_GetTextureSize(nameTex, &nw, &nh);
                        SDL_FRect nr = {dx - nw/2, dy + 32, nw, nh};
                        SDL_RenderTexture(renderer_, nameTex, NULL, &nr);
                        SDL_DestroyTexture(nameTex);
                    }
                }
            }

            // Ready indicator
            if (e.isReady && e.playerId >= 0 && texCache_) {
                auto* tick = texCache_->load("AirwarCPP\\Resources\\images\\ready.png");
                if (tick) spriteRenderer_->draw(tick, dx, dy - 48);
            }

            // Local player status bar
            if (e.playerId == localPlayerId) {
                drawHPBar(e.name, e.health, e.magabombQty);
            }
        }

        // Titles (with fade)
        for (auto& t : titles_) {
            float fade = t.delay <= GAME_TICK ? (float)t.delay / GAME_TICK : 1.0f;
            if (fade <= 0) continue;
            if (fontReady_ && fontLarge_) {
                SDL_Color c = {19, 19, 19, (Uint8)(fade * 255)};
                auto* surf = TTF_RenderText_Blended(fontLarge_, t.text.c_str(),
                    t.text.size(), c);
                if (surf) {
                    auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
                    SDL_DestroySurface(surf);
                    if (tex) {
                        float tw, th; SDL_GetTextureSize(tex, &tw, &th);
                        SDL_FRect r = {(width_ - tw)/2, (float)(height_/4 - th/2), tw, th};
                        SDL_RenderTexture(renderer_, tex, NULL, &r);
                        SDL_DestroyTexture(tex);
                    }
                }
            }
        }

        // Level info
        if (titles_.empty() && !levelInfo_.empty() && fontReady_ && fontSmall_) {
            SDL_Color c = {19, 19, 19, 255};
            auto* surf = TTF_RenderText_Blended(fontSmall_, levelInfo_.c_str(),
                levelInfo_.size(), c);
            if (surf) {
                auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
                SDL_DestroySurface(surf);
                if (tex) {
                    float tw, th; SDL_GetTextureSize(tex, &tw, &th);
                    SDL_FRect r = {(width_ - tw)/2, 8, tw, th};
                    SDL_RenderTexture(renderer_, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                }
            }
        }

        // Version in top-right
        if (fontReady_ && fontSmall_) {
            SDL_Color c = {19, 19, 19, 255};
            auto* surf = TTF_RenderText_Blended(fontSmall_, version_.c_str(),
                version_.size(), c);
            if (surf) {
                auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
                SDL_DestroySurface(surf);
                if (tex) {
                    float tw, th; SDL_GetTextureSize(tex, &tw, &th);
                    SDL_FRect r = {(float)(width_ - 8 - tw), 4, tw, th};
                    SDL_RenderTexture(renderer_, tex, NULL, &r);
                    SDL_DestroyTexture(tex);
                }
            }
        }
    }

private:
    void drawHPBar(const std::string& name, float hp, int bombs) {
        const int barY = height_ - 64;
        if (!fontReady_ || !fontSmall_) return;

        // Background bar
        SDL_SetRenderDrawColor(renderer_, 192, 192, 192, 255);
        SDL_FRect bgRect = {0, (float)barY, (float)width_, 64};
        SDL_RenderFillRect(renderer_, &bgRect);

        // Player name
        SDL_Color c = {19, 19, 19, 255};
        auto* nameSurf = TTF_RenderText_Blended(fontSmall_, ("Player " + name).c_str(),
            ("Player " + name).size(), c);
        if (nameSurf) {
            auto* nTex = SDL_CreateTextureFromSurface(renderer_, nameSurf);
            SDL_DestroySurface(nameSurf);
            if (nTex) {
                float nw, nh; SDL_GetTextureSize(nTex, &nw, &nh);
                SDL_FRect nr = {0, (float)(barY+4), nw, nh};
                SDL_RenderTexture(renderer_, nTex, NULL, &nr);
                SDL_DestroyTexture(nTex);
            }
        }

        // "HP:" label
        auto* hpSurf = TTF_RenderText_Blended(fontSmall_, "HP:", 3, c);
        if (hpSurf) {
            auto* hpTex = SDL_CreateTextureFromSurface(renderer_, hpSurf);
            SDL_DestroySurface(hpSurf);
            if (hpTex) {
                float hw, hh; SDL_GetTextureSize(hpTex, &hw, &hh);
                SDL_FRect hr = {0, (float)(barY+24), hw, hh};
                SDL_RenderTexture(renderer_, hpTex, NULL, &hr);
                SDL_DestroyTexture(hpTex);
            }
        }

        // HP bar border and fill
        SDL_SetRenderDrawColor(renderer_, 19, 19, 19, 255);
        SDL_FRect hpBorder = {35, (float)(barY+25), 202, 18};
        SDL_RenderFillRect(renderer_, &hpBorder);
        float ratio = std::max(0.0f, std::min(1.0f, hp / 100.0f));
        SDL_SetRenderDrawColor(renderer_,
            (Uint8)(255*(1-ratio)), (Uint8)(255*ratio), 0, 255);
        if (ratio > 0) {
            SDL_FRect hpFill = {36, (float)(barY+26), (float)(ratio*200), 16};
            SDL_RenderFillRect(renderer_, &hpFill);
        }

        // Magabomb icons (textured)
        if (texCache_) {
            auto* bombTex = texCache_->load("AirwarCPP\\Resources\\images\\item_maga.png");
            if (bombTex) {
                float bw, bh; SDL_GetTextureSize(bombTex, &bw, &bh);
                for (int i = 0; i < bombs && i < 10; ++i) {
                    SDL_FRect r = {(float)(width_ - 24 - i*28), (float)(barY+18), bw, bh};
                    SDL_RenderTexture(renderer_, bombTex, NULL, &r);
                }
            }
        }
    }
};
