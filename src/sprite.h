#ifndef SPRITE_H
#define SPRITE_H

#ifndef TESTING_HEADLESS
#include <raylib.h>
#endif

/* Animation IDs — state-based anims + attack-based anims */
typedef enum {
    ANIM_IDLE = 0,
    ANIM_WALK_FWD,
    ANIM_WALK_BACK,
    ANIM_CROUCH,
    ANIM_JUMP,
    ANIM_DASH_FWD,
    ANIM_DASH_BACK,
    ANIM_HITSTUN,
    ANIM_BLOCKSTUN,
    ANIM_KNOCKDOWN,
    ANIM_ATTACK_BASE = 16,  /* + NORMAL_5L/5M/5H/2L/2M/2H/JL/JM/JH (16-24) */
    ANIM_SPECIAL_BASE = 25, /* + specials: 236L/M/H, 623L/M/H, 214L/M/H, j.214L/M/H (25-36) */
    ANIM_SUPER_BASE = 37,   /* + supers (37-39) */
    ANIM_COUNT = 48
} AnimId;

typedef struct {
    int x, y;              /* top-left of first frame in sheet */
    int frame_width;       /* px width of one frame */
    int frame_height;      /* px height of one frame */
    int frame_count;       /* number of frames in row */
    int frame_duration;    /* game frames per anim frame */
    int looping;           /* 1=loop, 0=clamp to last */
} Animation;

typedef struct {
#ifndef TESTING_HEADLESS
    Texture2D sheet;       /* single composite texture */
#endif
    Animation anims[ANIM_COUNT];
    int loaded;            /* 1 if sheet loaded successfully */
} SpriteSet;

/* Load sprite sheet + manifest for a character (e.g. "ryker") */
void sprite_load(SpriteSet *set, const char *char_name);

/* Unload sprite sheet */
void sprite_unload(SpriteSet *set);

/* Draw a sprite frame. facing: 1=right, -1=left. Anchored bottom-center. */
void sprite_draw(const SpriteSet *set, int anim_id, int frame_index,
                 int x, int y, int facing, int dest_w, int dest_h);

/* Sync cached anim params from SpriteSet into CharacterState anim fields.
 * Called by game.c after player_update when anim changes. */
void sprite_sync_anim(const SpriteSet *set, int current_anim,
                      int *anim_frame, int *anim_timer,
                      int *anim_frame_duration, int *anim_frame_count,
                      int *anim_looping);

#endif /* SPRITE_H */
