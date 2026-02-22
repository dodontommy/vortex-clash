cmake_minimum_required(VERSION 3.16)
project(vortex_clash VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

set(TARGETS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} src/main.c src/game.c src/player.c src/render.c src/input.c src/hitbox.c src/combo.c src/character.c src/sprite.c src/projectile.c)
target_link_libraries(${PROJECT_NAME} raylib)

# Copy assets to build output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

# Test executable (no raylib dependency — headless)
add_executable(test_states tests/test_states.c src/player.c src/hitbox.c src/combo.c src/character.c src/input.c src/projectile.c)
target_compile_definitions(test_states PRIVATE TESTING_HEADLESS)
enable_testing()
add_test(NAME state_tests COMMAND test_states)
