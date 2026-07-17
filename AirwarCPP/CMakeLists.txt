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

add_executable(AirwarCPP main.cpp Core/Entity.cpp Core/Level.cpp Game/Item.cpp Game/Board.cpp Game/Game.cpp Render/Background.cpp Render/Particle.cpp)

target_include_directories(AirwarCPP PRIVATE ${SDL3_INCLUDE_DIRS})
target_link_directories(AirwarCPP PRIVATE
    ${SDL3_LIB_DIR}
    ${SDL3_IMAGE_LIB_DIR}
    ${SDL3_MIXER_LIB_DIR}
    ${SDL3_TTF_LIB_DIR}
)
target_include_directories(AirwarCPP PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(AirwarCPP PRIVATE
    SDL3
    SDL3_image
    SDL3_mixer
    SDL3_ttf
)

function(copy_dlls TARGET)
    set(DLL_FILES
        "${SDL3_LIB_DIR}/SDL3.dll"
        "${SDL3_IMAGE_LIB_DIR}/SDL3_image.dll"
        "${SDL3_MIXER_LIB_DIR}/SDL3_mixer.dll"
        "${SDL3_TTF_LIB_DIR}/SDL3_ttf.dll"
    )
    foreach(dll ${DLL_FILES})
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${TARGET}>"
            COMMENT "Copying ${dll}"
        )
    endforeach()
endfunction()
copy_dlls(AirwarCPP)

target_compile_definitions(AirwarCPP PRIVATE
    SDL_ARCH="${SDL_ARCH}"
)
