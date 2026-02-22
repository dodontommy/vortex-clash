#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "input.h"
#include "combo.h"

/* Cancel hierarchy: what can cancel the current state? */
typedef enum {
    CANCEL_FREE       = 0,  /* Anything (idle, walk, crouch) */
    CANCEL_BY_NORMAL  = 1,  /* Normals, specials, supers, two-button dash */
    CANCEL_BY_SPECIAL = 2,  /* Specials and supers only */
    CANCEL_BY_SUPER   = 3,  /* Supers only */
    CANCEL_NONE       = 4   /* Nothing */
} CancelLevel;

/* Action type: what is the incoming action? */
typedef enum {
    ACTION_MOVEMENT = 0,  /* Walk, crouch, jump */
    ACTION_NORMAL   = 1,  /* Normal attacks, two-button dash */
    ACTION_SPECIAL  = 2,  /* Special moves */
    ACTION_SUPER    = 3   /* Super moves */
} ActionType;

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
    STATE_ATTACK_STARTUP,
    STATE_ATTACK_ACTIVE,
    STATE_ATTACK_RECOVERY,
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
    int standing_height;
    int crouch_height;
    int color_r, color_g, color_b;
    int facing;             /* -1 = left, 1 = right */
    bool_t on_ground;
    bool_t dashing;
    bool_t crouching;
    bool_t in_hitstun;
    bool_t in_blockstun;
    /* Combat stats */
    int hp;
    int hitstun_remaining;
    int blockstun_remaining;
    int max_hp;
    /* Animation tracking */
    int current_anim;         /* AnimId */
    int anim_frame;           /* current frame index */
    int anim_timer;           /* game frames until next anim frame */
    int anim_frame_duration;  /* cached from Animation for advance_anim */
    int anim_frame_count;     /* cached frame count */
    int anim_looping;         /* cached looping flag */
    /* Hit juice */
    int hit_flash;            /* frames of white flash remaining */
} CharacterState;

/* Forward declaration */
struct MoveData;

/* Player wrapper with input */
typedef struct {
    CharacterState chars[2];  /* Two characters on screen */
    int active_char;          /* 0 = point, 1 = assist */
    int player_id;            /* 1 or 2 */
    int character_id;         /* CharacterId (int to avoid circular include) */
    int last_input_dir;       /* Last direction input for double-tap detection */
    int dir_change_frame;     /* Frame when direction last changed */
    uint32_t prev_input;      /* Previous frame's raw input for edge detection */
    int frame_counter;        /* Frame counter for this player */
    /* Attack tracking */
    const struct MoveData *current_attack;
    int attack_hit_id;        /* ID of last hit to prevent multi-hits */
    int opponent_hits[2];     /* Which opponent IDs have been hit this attack */
    /* Cancel hierarchy */
    bool_t hit_confirmed;     /* Set on hit (not block), cleared on new attack */
    bool_t attack_from_crouch; /* Attack started from crouching */
    bool_t attack_from_air;    /* Attack started from airborne */
    int button_press_frame[3]; /* Last frame L/M/H was pressed (for two-button dash leniency) */
    uint32_t pending_attack;       /* Buffered attack button from idle (0 = none) */
    int pending_attack_frame;      /* Frame when buffered */
    uint32_t pending_attack_dir;   /* Directional input at press time (for correct stand/crouch resolution) */
    uint32_t buffered_button;  /* Combo buffer: button pressed during non-cancelable state */
    int buffered_button_frame; /* Frame when buffered */
    /* Combo tracking */
    ComboState combo;         /* Current combo state */
    int meter;                /* Super meter (0-5000) */
    int blue_hp;              /* Recoverable health on assist */
} PlayerState;

/* Player functions */
void player_init(PlayerState *p, int player_id, int start_x, int start_y, int char_id);
void player_update(PlayerState *p, uint32_t input, const InputBuffer *input_buf);
void player_render(const PlayerState *p);
void player_update_facing(PlayerState *p1, PlayerState *p2);
void player_resolve_collisions(PlayerState *p1, PlayerState *p2);
void advance_anim(CharacterState *c);

#endif /* PLAYER_H */