set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SDL_ARCH "x64")
else()
    set(SDL_ARCH "x86")
endif()

set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3-3.4.12")
set(SDL3_IMAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3_image-3.4.4")
set(SDL3_MIXER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3_mixer-3.2.4")
set(SDL3_TTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL3_ttf-3.2.2")

set(SDL3_INCLUDE_DIRS
    "${SDL3_DIR}/include"
    "${SDL3_IMAGE_DIR}/include"
    "${SDL3_MIXER_DIR}/include"
    "${SDL3_TTF_DIR}/include"
)

set(SDL3_LIB_DIR "${SDL3_DIR}/lib/${SDL_ARCH}")
set(SDL3_IMAGE_LIB_DIR "${SDL3_IMAGE_DIR}/lib/${SDL_ARCH}")
set(SDL3_MIXER_LIB_DIR "${SDL3_MIXER_DIR}/lib/${SDL_ARCH}")
set(SDL3_TTF_LIB_DIR "${SDL3_TTF_DIR}/lib/${SDL_ARCH}")

# libuv bundled
set(LIBUV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libuv")
set(LIBUV_INCLUDE_DIR "${LIBUV_DIR}/include")
set(LIBUV_LIB_DIR "${LIBUV_DIR}/lib")

# Common source files (game logic, shared across client and server)
set(COMMON_SOURCES
    Core/Entity.cpp
    Core/Level.cpp
    Game/Item.cpp
    Game/Board.cpp
    Game/Game.cpp
)

# Common includes
set(COMMON_INCLUDES
    ${SDL3_INCLUDE_DIRS}
    ${LIBUV_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Common link dirs
set(COMMON_LINK_DIRS
    ${SDL3_LIB_DIR}
    ${SDL3_IMAGE_LIB_DIR}
    ${SDL3_MIXER_LIB_DIR}
    ${SDL3_TTF_LIB_DIR}
    ${LIBUV_LIB_DIR}
)

# Common libraries
set(COMMON_LIBS
    SDL3
    SDL3_image
    SDL3_mixer
    SDL3_ttf
    uv
)

# ── Function: copy DLLs (select which SDL libs) ──
function(copy_dlls TARGET)
    set(DLL_FILES
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${LIBUV_DIR}/bin/uv.dll"
    )
    # Only copy SDL_image if the target links it (check for flag)
    if(ARGV1 STREQUAL "NO_IMAGE")
        # skip
    else()
        list(APPEND DLL_FILES "${SDL3_IMAGE_LIB_DIR}/SDL3_image.dll")
    endif()
    if(ARGV1 STREQUAL "NO_MIXER" OR ARGV2 STREQUAL "NO_MIXER")
        # skip
    else()
        list(APPEND DLL_FILES "${SDL3_MIXER_LIB_DIR}/SDL3_mixer.dll")
    endif()
    if(ARGV1 STREQUAL "NO_TTF" OR ARGV2 STREQUAL "NO_TTF" OR ARGV3 STREQUAL "NO_TTF")
        # skip
    else()
        list(APPEND DLL_FILES "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll")
    endif()
    foreach(dll ${DLL_FILES})
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${TARGET}>"
            COMMENT "Copying ${dll}"
        )
    endforeach()
endfunction()

# ── Function: copy Resources to output directory ──
function(copy_resources TARGET)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_FILE_DIR:${TARGET}>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
            "$<TARGET_FILE_DIR:${TARGET}>/Resources"
        COMMENT "Copying Resources/"
    )
endfunction()

# ============================================================
# Main game executable (test suite)
# ============================================================
add_executable(AirwarCPP main.cpp ${COMMON_SOURCES}
    Render/Background.cpp Render/Particle.cpp
    Audio/SoundEngine.h Audio/MusicPlayer.h
    Config/ConfigPersistence.h
    UI/InputHandler.h UI/HUD.h UI/GameStateManager.h UI/Tutorial.h
)
target_include_directories(AirwarCPP PRIVATE ${COMMON_INCLUDES})
target_link_directories(AirwarCPP PRIVATE ${COMMON_LINK_DIRS})
target_link_libraries(AirwarCPP PRIVATE ${COMMON_LIBS})
copy_dlls(AirwarCPP)
target_compile_definitions(AirwarCPP PRIVATE SDL_ARCH="${SDL_ARCH}")

# ============================================================
# Airwar.exe — Client (console, but hides window at startup)
# ============================================================
# Client only needs SDL3 and SDL3_image (no mixer, no ttf)
set(CLIENT_LIBS SDL3 SDL3_image SDL3_ttf uv)
set(CLIENT_LINK_DIRS ${SDL3_LIB_DIR} ${SDL3_IMAGE_LIB_DIR} ${SDL3_TTF_LIB_DIR} ${LIBUV_LIB_DIR})

add_executable(Airwar Entry/client_main.cpp ${COMMON_SOURCES}
    Render/Background.cpp Render/Particle.cpp
    UI/InputHandler.h UI/HUD.h
)
target_include_directories(Airwar PRIVATE ${COMMON_INCLUDES})
target_link_directories(Airwar PRIVATE ${CLIENT_LINK_DIRS})
target_link_libraries(Airwar PRIVATE ${CLIENT_LIBS})
copy_dlls(Airwar NO_MIXER)
copy_resources(Airwar)
target_compile_definitions(Airwar PRIVATE SDL_ARCH="${SDL_ARCH}")

# ============================================================
# server.exe — Headless server (console subsystem)
# ============================================================
add_executable(server Entry/server_main.cpp ${COMMON_SOURCES})
target_include_directories(server PRIVATE ${COMMON_INCLUDES})
target_link_directories(server PRIVATE ${COMMON_LINK_DIRS})
target_link_libraries(server PRIVATE ${COMMON_LIBS})
copy_dlls(server)
copy_resources(server)
target_compile_definitions(server PRIVATE SDL_ARCH="${SDL_ARCH}")
