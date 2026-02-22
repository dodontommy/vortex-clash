#include "projectile.h"
#include "player.h"
#include "character.h"
#include <string.h>

void projectile_init(ProjectileState *ps) {
    memset(ps, 0, sizeof(*ps));
}

int projectile_spawn(ProjectileState *ps, int owner, const PlayerState *p, const MoveData *move, const CharacterState *defender) {
    /* One projectile per player — check if owner already has one active */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (ps->projectiles[i].active && ps->projectiles[i].owner == owner)
            return 0;
    }

    /* Find empty slot */
    int slot = -1;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!ps->projectiles[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return 0;

    const CharacterState *c = &p->chars[p->active_char];
    Projectile *proj = &ps->projectiles[slot];

    proj->active = TRUE;
    proj->owner = owner;
    proj->facing = c->facing;

    /* Spawn position: character pos + move offsets (facing-aware) */
    int x_off = c->facing == 1 ? move->x_offset : -move->x_offset - move->width;
    proj->x = c->x + FIXED_FROM_INT(x_off);
    proj->y = c->y + FIXED_FROM_INT(move->y_offset);

    /* Clamp: don't spawn past the defender (prevents fireball going through at close range).
     * Place projectile so its leading edge touches the defender's front edge. */
    if (defender) {
        if (c->facing == 1) {
            /* Facing right: projectile right edge shouldn't pass defender's left edge */
            fixed_t max_x = defender->x - FIXED_FROM_INT(move->width);
            if (proj->x > max_x) proj->x = max_x;
        } else {
            /* Facing left: projectile left edge shouldn't pass defender's right edge */
            fixed_t min_x = defender->x + FIXED_FROM_INT(defender->width);
            if (proj->x < min_x) proj->x = min_x;
        }
    }

    /* Velocity based on strength */
    fixed_t speed;
    switch (move->strength) {
        case 1:  speed = PROJECTILE_SPEED_M; break;
        case 2:  speed = PROJECTILE_SPEED_H; break;
        default: speed = PROJECTILE_SPEED_L; break;
    }
    proj->vx = speed * c->facing;

    /* Copy hitbox/hit data from MoveData */
    proj->width = move->width;
    proj->height = move->height;
    proj->damage = move->damage;
    proj->hitstun = move->hitstun;
    proj->blockstun = move->blockstun;
    proj->chip_damage = move->chip_damage;
    proj->hit_type = move->hit_type;
    proj->knockback_x = move->knockback_x;
    proj->knockback_y = move->knockback_y;
    proj->properties = move->properties;
    proj->strength = move->strength;
    proj->lifetime = PROJECTILE_LIFETIME;
    proj->hit_id = p->attack_hit_id;

    return 1;
}

void projectile_update(ProjectileState *ps) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *proj = &ps->projectiles[i];
        if (!proj->active) continue;

        /* Move */
        proj->x += proj->vx;

        /* Decrement lifetime */
        proj->lifetime--;

        /* Despawn if lifetime expired or out of stage bounds */
        int px = FIXED_TO_INT(proj->x);
        if (proj->lifetime <= 0 || px + proj->width < 0 || px > STAGE_WIDTH) {
            proj->active = FALSE;
        }
    }
}
