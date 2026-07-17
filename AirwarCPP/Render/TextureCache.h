#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <string>
#include <unordered_map>
#include <memory>

struct SDL_TextureDeleter {
    void operator()(SDL_Texture* t) const { if (t) SDL_DestroyTexture(t); }
};

class TextureCache {
    SDL_Renderer* renderer_ = nullptr;
    std::unordered_map<std::string, std::unique_ptr<SDL_Texture, SDL_TextureDeleter>> textures_;
public:
    void init(SDL_Renderer* r) { renderer_ = r; }

    SDL_Texture* load(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) return it->second.get();
        auto* surf = IMG_Load(path.c_str());
        if (!surf) { SDL_Log("IMG_Load(%s) failed: %s", path.c_str(), SDL_GetError()); return nullptr; }
        auto* tex = SDL_CreateTextureFromSurface(renderer_, surf);
        SDL_DestroySurface(surf);
        if (!tex) { SDL_Log("SDL_CreateTextureFromSurface failed: %s", SDL_GetError()); return nullptr; }
        textures_[path] = std::unique_ptr<SDL_Texture, SDL_TextureDeleter>(tex);
        return tex;
    }

    void clear() { textures_.clear(); }
};
