#ifndef HITBOX_H
#define HITBOX_H

#include "types.h"
#include "player.h"

/* Hitbox/Hurtbox rectangles */
typedef struct {
    int x, y;        /* Offset from character position */
    int width, height;
    int damage;
    int hitstun;
    int blockstun;
    int chip_damage;
    fixed_t knockback_x;
    fixed_t knockback_y;
    int hit_id;      /* Unique ID to prevent multi-hits */
    int owner_id;    /* Which player owns this hitbox */
} Hitbox;

typedef struct {
    int x, y;
    int width, height;
} Hurtbox;

/* Attack move definition */
typedef struct {
    int startup_frames;
    int active_start;    /* First active frame relative to startup end */
    int active_end;      /* Last active frame */
    int recovery_frames;
    int damage;
    int hitstun;
    int blockstun;
    int chip_damage;
    fixed_t knockback_x;
    fixed_t knockback_y;
    int x_offset;       /* Hitbox position relative to character */
    int y_offset;
    int width;
    int height;
} AttackMove;

/* Pre-defined attacks */
extern const AttackMove ATTACK_5L;  /* Standing light */
extern const AttackMove ATTACK_5M;  /* Standing medium */
extern const AttackMove ATTACK_5H;  /* Standing heavy */

/* Hitbox functions */
void hitbox_init(void);
int hitbox_check_collision(const Hitbox *hb, const Hurtbox *hurt, int char_x, int char_y, int facing);
void hitbox_resolve_hit(CharacterState *attacker, CharacterState *defender, const AttackMove *move, int is_blocking);
int hurtbox_create(CharacterState *c, Hurtbox *hurt);

/* Pushbox collision */
void pushbox_resolve(CharacterState *c1, CharacterState *c2);

/* Hitstop */
extern int g_hitstop_frames;
void hitbox_apply_hitstop(int frames);
void hitbox_update_hitstop(void);
int hitbox_in_hitstop(void);

/* Check if defender is blocking */
int is_blocking(CharacterState *defender, CharacterState *attacker, uint32_t input);

#endif /* HITBOX_H */