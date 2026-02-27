#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "types.h"
#include "player.h"
#include "character.h"

/* Limits */
#define MAX_PROJECTILES     4   /* 2 players x room for future multi-projectile chars */
#define PROJECTILE_LIFETIME 120 /* 2 seconds at 60fps */

/* Speed per strength level */
#define PROJECTILE_SPEED_L  FIXED_FROM_INT(5)
#define PROJECTILE_SPEED_M  FIXED_FROM_INT(7)
#define PROJECTILE_SPEED_H  FIXED_FROM_INT(9)

typedef struct {
    bool_t active;
    int owner;              /* player index 0 or 1 */
    fixed_t x, y;           /* world position (absolute) */
    fixed_t vx;             /* horizontal velocity (includes facing direction) */
    int width, height;      /* hitbox size */
    int damage;
    int hitstun;
    int blockstun;
    int chip_damage;
    int meter_gain;
    int hit_type;
    fixed_t knockback_x, knockback_y;
    int properties;         /* copied from MoveData */
    int lifetime;           /* frames remaining */
    int hit_id;
    int facing;
    int strength;           /* 0/1/2 for L/M/H — affects speed + color */
} Projectile;

typedef struct {
    Projectile projectiles[MAX_PROJECTILES];
} ProjectileState;

/* Functions */
void projectile_init(ProjectileState *ps);
int  projectile_spawn(ProjectileState *ps, int owner, const PlayerState *p, const MoveData *move, const CharacterState *defender);
void projectile_update(ProjectileState *ps);

#endif /* PROJECTILE_H */
