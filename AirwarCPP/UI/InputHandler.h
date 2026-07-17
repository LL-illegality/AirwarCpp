#pragma once
#include "../Core/Constants.h"
#include <SDL3/SDL.h>
#include <vector>
#include <functional>

enum class GameAction {
    MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT,
    SHOOT, PREPARE, MAGABOMB, DRAW_MARKER, PAUSE, UNKNOWN
};

struct InputState {
    bool moveUp = false, moveDown = false, moveLeft = false, moveRight = false;
    bool shoot = false, prepare = false, magabomb = false, drawMarker = false;
    bool pause = false;

    std::vector<int> toPressedKeyList() const {
        std::vector<int> keys;
        if (moveUp) keys.push_back(Keys::w);
        if (moveDown) keys.push_back(Keys::s);
        if (moveLeft) keys.push_back(Keys::a);
        if (moveRight) keys.push_back(Keys::d);
        if (shoot) keys.push_back(Keys::space);
        if (prepare) keys.push_back(Keys::c);
        return keys;
    }

    void clear() {
        moveUp = moveDown = moveLeft = moveRight = false;
        shoot = prepare = magabomb = drawMarker = pause = false;
    }
};

class InputHandler {
    InputState state_;
    bool gamepadConnected_ = false;

public:
    InputHandler() {
        int count = 0;
        auto* ids = SDL_GetJoysticks(&count);
        if (ids) { SDL_free(ids); }
        for (int i = 0; i < count; ++i) {
            if (SDL_IsGamepad(i)) { gamepadConnected_ = true; break; }
        }
    }

    const InputState& state() const { return state_; }
    bool gamepadConnected() const { return gamepadConnected_; }

    GameAction mapKey(SDL_Keycode key) const {
        if (key == SDLK_W || key == SDLK_UP)    return GameAction::MOVE_UP;
        if (key == SDLK_S || key == SDLK_DOWN)  return GameAction::MOVE_DOWN;
        if (key == SDLK_A || key == SDLK_LEFT)  return GameAction::MOVE_LEFT;
        if (key == SDLK_D || key == SDLK_RIGHT) return GameAction::MOVE_RIGHT;
        if (key == SDLK_SPACE) return GameAction::SHOOT;
        if (key == SDLK_C)     return GameAction::PREPARE;
        if (key == SDLK_E)     return GameAction::MAGABOMB;
        if (key == SDLK_Z)     return GameAction::DRAW_MARKER;
        if (key == SDLK_P || key == SDLK_ESCAPE) return GameAction::PAUSE;
        return GameAction::UNKNOWN;
    }

    GameAction mapGamepadButton(Uint8 button) const {
        if (button == 0) return GameAction::PREPARE;      // A / cross
        if (button == 1) return GameAction::DRAW_MARKER;  // B / circle
        if (button == 2) return GameAction::MAGABOMB;     // X / square
        if (button == 3) return GameAction::SHOOT;         // Y / triangle
        if (button == 7) return GameAction::PAUSE;         // Start
        return GameAction::UNKNOWN;
    }

    void handleKeyDown(SDL_Keycode key) {
        switch (mapKey(key)) {
            case GameAction::MOVE_UP:    state_.moveUp = true; break;
            case GameAction::MOVE_DOWN:  state_.moveDown = true; break;
            case GameAction::MOVE_LEFT:  state_.moveLeft = true; break;
            case GameAction::MOVE_RIGHT: state_.moveRight = true; break;
            case GameAction::SHOOT:      state_.shoot = true; break;
            case GameAction::PREPARE:    state_.prepare = true; break;
            case GameAction::MAGABOMB:   state_.magabomb = true; break;
            case GameAction::DRAW_MARKER: state_.drawMarker = true; break;
            case GameAction::PAUSE:      state_.pause = true; break;
            default: break;
        }
    }

    void handleKeyUp(SDL_Keycode key) {
        switch (mapKey(key)) {
            case GameAction::MOVE_UP:    state_.moveUp = false; break;
            case GameAction::MOVE_DOWN:  state_.moveDown = false; break;
            case GameAction::MOVE_LEFT:  state_.moveLeft = false; break;
            case GameAction::MOVE_RIGHT: state_.moveRight = false; break;
            case GameAction::SHOOT:      state_.shoot = false; break;
            case GameAction::PREPARE:    state_.prepare = false; break;
            case GameAction::MAGABOMB:   state_.magabomb = false; break;
            case GameAction::DRAW_MARKER: state_.drawMarker = false; break;
            case GameAction::PAUSE:      state_.pause = false; break;
            default: break;
        }
    }

    void handleGamepadButtonDown(Uint8 button) {
        switch (mapGamepadButton(button)) {
            case GameAction::SHOOT:      state_.shoot = true; break;
            case GameAction::PREPARE:    state_.prepare = true; break;
            case GameAction::MAGABOMB:   state_.magabomb = true; break;
            case GameAction::DRAW_MARKER: state_.drawMarker = true; break;
            case GameAction::PAUSE:      state_.pause = true; break;
            default: break;
        }
    }

    void handleGamepadButtonUp(Uint8 button) {
        switch (mapGamepadButton(button)) {
            case GameAction::SHOOT:      state_.shoot = false; break;
            case GameAction::PREPARE:    state_.prepare = false; break;
            case GameAction::MAGABOMB:   state_.magabomb = false; break;
            case GameAction::DRAW_MARKER: state_.drawMarker = false; break;
            case GameAction::PAUSE:      state_.pause = false; break;
            default: break;
        }
    }

    void handleGamepadAxis(int axis, float value) {
        const float deadZone = 0.2f;
        if (axis == 0) {
            state_.moveLeft = value < -deadZone;
            state_.moveRight = value > deadZone;
        } else if (axis == 1) {
            state_.moveUp = value < -deadZone;
            state_.moveDown = value > deadZone;
        }
    }

    void handleGamepadHat(Uint8 hat, Uint8 value) {
        state_.moveUp = (value & SDL_HAT_UP) != 0;
        state_.moveDown = (value & SDL_HAT_DOWN) != 0;
        state_.moveLeft = (value & SDL_HAT_LEFT) != 0;
        state_.moveRight = (value & SDL_HAT_RIGHT) != 0;
    }
};
