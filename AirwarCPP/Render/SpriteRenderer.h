#pragma once
#include <SDL3/SDL.h>

struct SpriteRenderer {
    SDL_Renderer* renderer = nullptr;

    void init(SDL_Renderer* r) { renderer = r; }

    void draw(SDL_Texture* tex, float x, float y, float rotation = 0,
              float alpha = 255, float scale = 1.0f) const {
        if (!tex) return;
        float w, h;
        SDL_GetTextureSize(tex, &w, &h);
        w *= scale; h *= scale;
        SDL_FRect dst = { x - w / 2, y - h / 2, w, h };
        if (alpha < 255) SDL_SetTextureAlphaMod(tex, (Uint8)alpha);
        if (rotation != 0)
            SDL_RenderTextureRotated(renderer, tex, NULL, &dst, -rotation, NULL, SDL_FLIP_NONE);
        else
            SDL_RenderTexture(renderer, tex, NULL, &dst);
        if (alpha < 255) SDL_SetTextureAlphaMod(tex, 255);
    }

    void drawInterpolated(SDL_Texture* tex, float prevX, float prevY, float currX, float currY,
                          float prevRot, float currRot, float alpha, float spriteAlpha = 255,
                          float scale = 1.0f) const {
        float x = prevX + (currX - prevX) * alpha;
        float y = prevY + (currY - prevY) * alpha;
        float rot = prevRot + (currRot - prevRot) * alpha;
        draw(tex, x, y, rot, spriteAlpha, scale);
    }
};
