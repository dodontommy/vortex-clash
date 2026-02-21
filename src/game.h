#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "player.h"
#include "input.h"

/* Game state with two players */
typedef struct {
    PlayerState p1;
    PlayerState p2;
    InputBuffer p1_input;
    InputBuffer p2_input;
    int frame_count;
    bool_t running;
} GameState;

/* Game functions */
void game_init(GameState *game);
void game_update(GameState *game);
void game_render(const GameState *game);
void game_shutdown(GameState *game);

#endif /* GAME_H */
