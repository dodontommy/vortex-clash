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

/* Per-frame move data */
typedef struct {
    int frame;                 /* Frame number (0-indexed from move start) */
    int hitbox_count;          /* Number of active hitboxes this frame */
    /* Hitboxes would be defined per-frame in a full implementation */
    int cancellable;           /* What cancels are available this frame */
    int flags;                 /* Properties like INVINCIBLE */
} MoveFrame;

/* Complete move definition */
typedef struct {
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
    int meter_cost;
    int meter_gain;
} MoveData;

/* Character definition */
typedef struct {
    CharacterId id;
    const char *name;
    
    /* Movement stats */
    fixed_t walk_speed_forward;
    fixed_t walk_speed_backward;
    fixed_t jump_velocity;
    fixed_t air_dash_speed;
    
    /* Health */
    int max_hp;
    int damage_multiplier;      /* 100 = normal, 90 = takes 90% damage (Titan) */
    
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

/* Helper macros for creating moves */
#define MOVE(n, type, total, startup, act_start, act_end, rec, dmg, hs, bs, chip, ht, kbx, kby, xo, yo, w, h, props) \
    { #n, type, total, startup, act_start, act_end, rec, dmg, hs, bs, chip, ht, kbx, kby, xo, yo, w, h, props, 0, 0 }

#endif /* CHARACTER_H */
