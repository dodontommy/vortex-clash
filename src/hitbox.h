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

/* Forward declaration */
struct MoveData;

/* Hitbox functions */
void hitbox_init(void);
int hitbox_check_collision(const Hitbox *hb, const Hurtbox *hurt, int char_x, int char_y, int facing);
void hitbox_resolve_hit(CharacterState *attacker, CharacterState *defender, const struct MoveData *move, int is_blocking,
                        ComboState *attacker_combo, ComboState *defender_combo, int *hitstop, int counter_hit);
int hurtbox_create(CharacterState *c, Hurtbox *hurt);

/* Hitstop — caller owns the counter (lives in GameState) */
void hitbox_apply_hitstop(int *hitstop, int frames);
void hitbox_update_hitstop(int *hitstop);
int hitbox_in_hitstop(const int *hitstop);

/* Check if defender is blocking */
int is_blocking(CharacterState *defender, CharacterState *attacker, uint32_t input);

#endif /* HITBOX_H */