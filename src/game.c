#include "game.h"
#include "player.h"
#include "render.h"
#include <raylib.h>

void game_init(GameState *game) {
    player_init(&game->p1, 1, 200, 400);
    player_init(&game->p2, 2, 900, 400);
    input_init(&game->p1_input);
    input_init(&game->p2_input);
    game->frame_count = 0;
    game->running = TRUE;
    render_init();
}

void game_update(GameState *game) {
    /* Poll inputs */
    uint32_t p1_raw = input_poll(1, INPUT_SOURCE_KEYBOARD);
    uint32_t p2_raw = input_poll(2, INPUT_SOURCE_KEYBOARD);
    
    /* Update input buffers */
    input_update(&game->p1_input, p1_raw);
    input_update(&game->p2_input, p2_raw);
    
    /* Use buffered input for players */
    player_update(&game->p1, input_get_current(&game->p1_input));
    player_update(&game->p2, input_get_current(&game->p2_input));
    player_update_facing(&game->p1, &game->p2);
    player_resolve_collisions(&game->p1, &game->p2);
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