#include "game.h"
#include "physics.h"
#include "render.h"
#include <raylib.h>
#include <stdlib.h>

extern Character g_player;

void game_init(GameState *game) {
    /* Allocate and initialize player */
    game->player = &g_player;
    physics_init(game->player, 100, 400, 50, 80, 255, 80, 80);

    game->frame_count = 0;
    game->running = TRUE;

    render_init();
}

void game_update(GameState *game) {
    Character *p = game->player;

    /* Input handling */
    p->vx = 0;
    if (IsKeyDown(KEY_LEFT)) {
        p->vx = -MOVE_SPEED;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        p->vx = MOVE_SPEED;
    }
    if (IsKeyPressed(KEY_UP) && p->on_ground) {
        p->vy = JUMP_VELOCITY;
        p->on_ground = FALSE;
    }

    /* Physics */
    physics_update(p);

    game->frame_count++;
}

void game_render(const GameState *game) {
    render_stage();
    render_character(game->player);
    render_frame_counter(game->frame_count);
}

void game_shutdown(GameState *game) {
    render_shutdown();
    game->player = NULL;
}
