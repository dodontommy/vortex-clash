cmake_minimum_required(VERSION 3.16)
project(vortex_clash VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(VORTEX_BUILD_GAME "Build the playable game executable" ON)
option(VORTEX_ALLOW_NET_FETCH "Allow CMake to fetch dependencies from the network" OFF)

set(TARGETS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

if(VORTEX_BUILD_GAME)
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        if(VORTEX_ALLOW_NET_FETCH)
            include(FetchContent)
            FetchContent_Declare(
                raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG 5.0
            )
            FetchContent_MakeAvailable(raylib)
        else()
            message(WARNING
                "raylib not found and network fetch is disabled. "
                "Disabling game target. Reconfigure with -DVORTEX_ALLOW_NET_FETCH=ON "
                "to fetch raylib automatically.")
            set(VORTEX_BUILD_GAME OFF)
        endif()
    endif()
endif()

if(VORTEX_BUILD_GAME)
    add_executable(${PROJECT_NAME} src/main.c src/game.c src/player.c src/render.c src/input.c src/hitbox.c src/combo.c src/character.c src/system_moves.c src/sprite.c src/projectile.c src/sfx.c)
    target_link_libraries(${PROJECT_NAME} raylib)

    # Copy assets to build output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)
endif()

# Test executable (no raylib dependency — headless)
add_executable(test_states tests/test_states.c src/player.c src/hitbox.c src/combo.c src/character.c src/system_moves.c src/input.c src/projectile.c)
target_compile_definitions(test_states PRIVATE TESTING_HEADLESS)
enable_testing()
add_test(NAME state_tests COMMAND test_states)
