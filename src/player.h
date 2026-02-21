#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"

/* Character state enum */
typedef enum {
    STATE_IDLE,
    STATE_WALK_FORWARD,
    STATE_WALK_BACKWARD,
    STATE_CROUCH,
    STATE_JUMP_SQUAT,
    STATE_AIRBORNE,
    STATE_LANDING,
    STATE_DASH_FORWARD,
    STATE_DASH_BACKWARD,
    STATE_ATTACK_LIGHT,
    STATE_ATTACK_MEDIUM,
    STATE_ATTACK_HEAVY,
    STATE_BLOCK_STANDING,
    STATE_BLOCK_CROUCHING,
    STATE_HITSTUN,
    STATE_BLOCKSTUN,
    STATE_KNOCKDOWN
} CharacterStateEnum;

/* Character state data */
typedef struct {
    CharacterStateEnum state;
    int state_frame;        /* Frames in current state */
    fixed_t x, y;
    fixed_t vx, vy;
    int width, height;
    int color_r, color_g, color_b;
    int facing;             /* -1 = left, 1 = right */
    bool_t on_ground;
    bool_t jumping;
    bool_t dashing;
    bool_t crouching;
    bool_t blocking;
    bool_t in_hitstun;
    bool_t in_blockstun;
} CharacterState;

/* Player wrapper with input */
typedef struct {
    CharacterState chars[2];  /* Two characters on screen */
    int active_char;          /* 0 = point, 1 = assist */
    int player_id;            /* 1 or 2 */
    int last_input_dir;       /* Last direction input for double-tap detection */
    int dir_change_frame;     /* Frame when direction last changed */
    int frame_counter;        /* Frame counter for this player */
} PlayerState;

/* Player functions */
void player_init(PlayerState *p, int player_id, int start_x, int start_y);
void player_update(PlayerState *p, uint32_t input);
void player_render(const PlayerState *p);
void player_update_facing(PlayerState *p1, PlayerState *p2);

/* Input button bits */
#define INPUT_UP     (1 << 0)
#define INPUT_DOWN   (1 << 1)
#define INPUT_LEFT   (1 << 2)
#define INPUT_RIGHT  (1 << 3)
#define INPUT_LIGHT  (1 << 4)
#define INPUT_MEDIUM (1 << 5)
#define INPUT_HEAVY  (1 << 6)
#define INPUT_ASSIST (1 << 7)

/* Helper to get input state */
#define INPUT_HAS(in, bit) (((in) & (bit)) != 0)

#endif /* PLAYER_H */