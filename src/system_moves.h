#ifndef SYSTEM_MOVES_H
#define SYSTEM_MOVES_H

#include "character.h"

typedef enum {
    SYSTEM_MOVE_SNAPBACK = 0,
    SYSTEM_MOVE_COUNT
} SystemMoveId;

/* Accessor for universal/non-character-specific moves. */
const MoveData *system_move_get(int system_move_id);

#endif /* SYSTEM_MOVES_H */
