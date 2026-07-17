#pragma once
#include <cstdint>

enum class ClientMsgType : uint8_t {
    connect    = 0,
    disconnect = 1,
    get        = 2,
    keyDown    = 3,
    keyUp      = 4,
    joyAxis    = 5,
    joyHat     = 6
};

enum class ServerMsgType : uint8_t {
    connect            = 0,
    screen_info        = 1,
    game_state_changed = 2,
    playsound          = 3,
    particle_effect    = 4,
    load_level         = 5,
    set_title          = 6
};
