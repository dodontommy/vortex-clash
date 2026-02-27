#ifndef CHARACTER_H
#define CHARACTER_H

#include "types.h"
#include "player.h"
#include "combo.h"

/* Character IDs */
typedef enum {
    CHAR_RYKER = 0,
    CHAR_ZARA,
    CHAR_TITAN,
    CHAR_VIPER,
    CHAR_AEGIS,
    CHAR_BLAZE,
    CHAR_COUNT
} CharacterId;

/* Move types */
typedef enum {
    MOVE_TYPE_NORMAL,
    MOVE_TYPE_SPECIAL,
    MOVE_TYPE_SUPER,
    MOVE_TYPE_THROW
} MoveType;

/* Hit types */
typedef enum {
    HIT_TYPE_HIGH,
    HIT_TYPE_MID,
    HIT_TYPE_LOW,
    HIT_TYPE_OVERHEAD,
    HIT_TYPE_UNBLOCKABLE
} HitType;

/* Move properties (bitmask) */
#define MOVE_PROP_NONE          0
#define MOVE_PROP_CHAIN         (1 << 0)   /* Can chain into next normal */
#define MOVE_PROP_SPECIAL_CANCEL (1 << 1)  /* Can cancel into special on hit */
#define MOVE_PROP_SUPER_CANCEL   (1 << 2)  /* Can cancel into super on hit */
#define MOVE_PROP_JUMP_CANCEL    (1 << 3)  /* Can jump cancel on hit/block */
#define MOVE_PROP_INVINCIBLE     (1 << 4)  /* Full invincibility */
#define MOVE_PROP_ARMOR          (1 << 5)  /* Armor (absorb 1 hit) */
#define MOVE_PROP_OTG            (1 << 6)  /* Can hit OTG */
#define MOVE_PROP_LAUNCHER       (1 << 7)  /* Launches opponent */
#define MOVE_PROP_WALL_BOUNCE    (1 << 8)  /* Causes wall bounce */
#define MOVE_PROP_GROUND_BOUNCE  (1 << 9)  /* Causes ground bounce */
#define MOVE_PROP_SLIDING_KD     (1 << 10) /* Causes sliding knockdown */
#define MOVE_PROP_HARD_KD        (1 << 11) /* Causes hard knockdown */
#define MOVE_PROP_PROJECTILE     (1 << 12) /* Is a projectile */
#define MOVE_PROP_LOW            (1 << 13) /* Low hit (crouching) */
#define MOVE_PROP_SUPER_JUMP_CANCEL (1 << 14) /* Super jump cancel on hit (S launcher) */

/* Per-frame move data */
typedef struct {
    int frame;                 /* Frame number (0-indexed from move start) */
    int hitbox_count;          /* Number of active hitboxes this frame */
    /* Hitboxes would be defined per-frame in a full implementation */
    int cancellable;           /* What cancels are available this frame */
    int flags;                 /* Properties like INVINCIBLE */
} MoveFrame;

/* Stance requirements (bitmask) — where a move can be used.
 * 0 means no restriction (usable anywhere, default for normals). */
#define STANCE_GROUNDED  (1 << 0)  /* Must be on the ground */
#define STANCE_AIRBORNE  (1 << 1)  /* Must be in the air */
#define STANCE_STANDING  (1 << 2)  /* Must be standing (not crouching) */
#define STANCE_ANY       0         /* No restriction */

/* Normal index constants */
#define NORMAL_5L  0
#define NORMAL_5M  1
#define NORMAL_5H  2
#define NORMAL_2L  3
#define NORMAL_2M  4
#define NORMAL_2H  5
#define NORMAL_JL  6
#define NORMAL_JM  7
#define NORMAL_JH  8
#define NORMAL_J2L 9   /* Air command normals: j.2+button (dive kicks, etc.) */
#define NORMAL_J2M 10
#define NORMAL_J2H 11
#define NORMAL_5S  12  /* Standing S (universal launcher) */
#define NORMAL_JS  13  /* Air S (air exchange / spikedown) */

/* Complete move definition */
typedef struct MoveData {
    const char *name;
    MoveType move_type;
    int total_frames;           /* Total duration */
    int startup_frames;        /* Before first active frame */
    int active_start;           /* First active frame (relative to startup end) */
    int active_end;             /* Last active frame */
    int recovery_frames;       /* After last active frame */
    
    /* Hit data */
    int damage;
    int hitstun;
    int blockstun;
    int chip_damage;
    HitType hit_type;
    
    /* Physics */
    fixed_t knockback_x;
    fixed_t knockback_y;
    
    /* Hitbox dimensions */
    int x_offset;
    int y_offset;
    int width;
    int height;
    
    /* Properties */
    int properties;
    int stance;             /* STANCE_* bitmask: where this move can be used (0 = anywhere) */
    int strength;           /* 0=Light, 1=Medium, 2=Heavy (for chain ordering) */
    int meter_cost;
    int meter_gain;
    int anim_id;            /* AnimId for sprite animation (-1 = none) */
    /* Throw-specific behavior (used when move_type == MOVE_TYPE_THROW). */
    int throw_range;        /* Horizontal front-facing connect range in pixels */
    int throw_damage_frame; /* Frame of throw sequence when damage is applied */
    int throw_duration;     /* Total throw lock duration in frames */
    int throw_side_switch;  /* 1 = swap sides by end of throw */
    const int *cancel_into; /* NULL = use properties flags, otherwise list of NORMAL_* indices terminated by -1 */
} MoveData;

/* Character definition */
typedef struct {
    CharacterId id;
    const char *name;
    
    /* Movement stats */
    fixed_t walk_speed_forward;
    fixed_t walk_speed_backward;
    fixed_t jump_velocity;
    fixed_t dash_speed;
    fixed_t air_dash_speed;
    fixed_t super_jump_velocity;
    
    /* Health */
    int max_hp;
    int damage_multiplier;      /* 100 = normal, 90 = takes 90% damage (Titan) */

    /* Per-character movement tuning */
    int jump_squat_frames;
    int dash_forward_frames;
    int dash_backward_frames;
    fixed_t dash_friction;
    fixed_t dash_min_speed;
    int landing_frames;

    /* Pushbox dimensions */
    int width;
    int standing_height;
    int crouch_height;

    /* Moves - indexed by input */
    const MoveData *normals[16];   /* 5L, 5M, 5H, 2L, 2M, 2H, j.L, j.M, j.H, etc */
    const MoveData *specials[16];  /* 236L, 236M, 236H, 214L, etc */
    const MoveData *supers[4];     /* Level 1-3 supers */
    const MoveData *throw_move;
    const MoveData *assist_move;
} CharacterDef;

/* Get character definition */
const CharacterDef *character_get_def(CharacterId id);

/* Get move from input */
const MoveData *character_get_normal(CharacterId id, int input);
const MoveData *character_get_special(CharacterId id, int motion, int button);
const MoveData *character_get_super(CharacterId id, int level);
const MoveData *character_get_throw(CharacterId id);
const MoveData *character_get_assist_move(CharacterId id);
const MoveData *character_get_move_by_slot(CharacterId id, int attack_ref_type, int index);

/* Helper macros for creating moves */
#define MOVE(n, type, total, startup, act_start, act_end, rec, dmg, hs, bs, chip, ht, kbx, kby, xo, yo, w, h, props) \
    { #n, type, total, startup, act_start, act_end, rec, dmg, hs, bs, chip, ht, kbx, kby, xo, yo, w, h, props, 0, 0, 0, 0, -1, 0, 0, 0, 0, NULL }

#endif /* CHARACTER_H */
