#include "system_moves.h"
#include <stddef.h>

/* Snapback move data — universal, costs 1 bar */
static const MoveData SNAPBACK_MOVE = {
    .name = "Snapback",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 40,
    .startup_frames = 20,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 17,
    .damage = 800,
    .hitstun = 20,
    .blockstun = 12,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(6),
    .knockback_y = 0,
    .x_offset = 50,
    .y_offset = 15,
    .width = 45,
    .height = 40,
    .properties = MOVE_PROP_NONE,
    .stance = STANCE_GROUNDED,
    .strength = 2,
    .meter_cost = 1000,
    .meter_gain = 0,
    .anim_id = -1
};

const MoveData *system_move_get(int system_move_id) {
    switch (system_move_id) {
        case SYSTEM_MOVE_SNAPBACK:
            return &SNAPBACK_MOVE;
        default:
            return NULL;
    }
}
