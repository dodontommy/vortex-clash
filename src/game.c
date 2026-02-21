#include "game.h"
#include "player.h"
#include "render.h"
#include <raylib.h>

static uint32_t get_p1_input(void) {
    uint32_t input = 0;
    if (IsKeyDown(KEY_W)) input |= INPUT_UP;
    if (IsKeyDown(KEY_S)) input |= INPUT_DOWN;
    if (IsKeyDown(KEY_A)) input |= INPUT_LEFT;
    if (IsKeyDown(KEY_D)) input |= INPUT_RIGHT;
    if (IsKeyDown(KEY_V)) input |= INPUT_LIGHT;
    if (IsKeyDown(KEY_B)) input |= INPUT_MEDIUM;
    return input;
}

static uint32_t get_p2_input(void) {
    uint32_t input = 0;
    if (IsKeyDown(KEY_UP)) input |= INPUT_UP;
    if (IsKeyDown(KEY_DOWN)) input |= INPUT_DOWN;
    if (IsKeyDown(KEY_LEFT)) input |= INPUT_LEFT;
    if (IsKeyDown(KEY_RIGHT)) input |= INPUT_RIGHT;
    if (IsKeyDown(KEY_KP_1)) input |= INPUT_LIGHT;
    if (IsKeyDown(KEY_KP_2)) input |= INPUT_MEDIUM;
    return input;
}

void game_init(GameState *game) {
    player_init(&game->p1, 1, 200, 400);
    player_init(&game->p2, 2, 900, 400);
    game->frame_count = 0;
    game->running = TRUE;
    render_init();
}

void game_update(GameState *game) {
    player_update(&game->p1, get_p1_input());
    player_update(&game->p2, get_p2_input());
    player_update_facing(&game->p1, &game->p2);
    game->frame_count++;
}

void game_render(const GameState *game) {
    render_stage();
    player_render(&game->p1);
    player_render(&game->p2);
    render_frame_counter(game->frame_count);
}

void game_shutdown(GameState *game) {
    render_shutdown();
}