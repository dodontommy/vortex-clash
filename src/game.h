#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "physics.h"

/* Simplified game state for Phase 2 */
typedef struct {
    Character *player;
    int frame_count;
    bool_t running;
} GameState;

/* Game functions */
void game_init(GameState *game);
void game_update(GameState *game);
void game_render(const GameState *game);
void game_shutdown(GameState *game);

#endif /* GAME_H */
