#include "hitbox.h"
#include "character.h"
#include "combo.h"
#include <string.h>

void hitbox_init(void) {
    /* Nothing to initialize — hitstop lives in GameState now */
}

/* Create hurtbox from character state */
int hurtbox_create(CharacterState *c, Hurtbox *hurt) {
    hurt->x = FIXED_TO_INT(c->x);
    hurt->y = FIXED_TO_INT(c->y);
    hurt->width = c->width;
    hurt->height = c->height;
    return 1;
}

/* Check collision between hitbox and hurtbox */
int hitbox_check_collision(const Hitbox *hb, const Hurtbox *hurt, int char_x, int char_y, int facing) {
    /* Calculate hitbox world position */
    int hb_x = char_x + (facing == 1 ? hb->x : -hb->x - hb->width);
    int hb_y = char_y + hb->y;

    /* AABB collision test */
    return (hb_x < hurt->x + hurt->width &&
            hb_x + hb->width > hurt->x &&
            hb_y < hurt->y + hurt->height &&
            hb_y + hb->height > hurt->y);
}

/* Check if defender is blocking (accounts for high/low/overhead).
 * Standard fighting game rules:
 *   MID / HIGH  — blocked by standing or crouching block
 *   LOW         — must block crouching (holding down-back)
 *   OVERHEAD    — must block standing (holding back, NOT down)
 *   UNBLOCKABLE — never blocked
 */
int is_blocking(CharacterState *defender, CharacterState *attacker, uint32_t input, int hit_type) {
    /* Unblockable moves can never be blocked */
    if (hit_type == HIT_TYPE_UNBLOCKABLE) return 0;

    /* Must be holding back (away from attacker) */
    int holding_back = 0;
    if (attacker->x < defender->x)
        holding_back = INPUT_HAS(input, INPUT_RIGHT);
    else
        holding_back = INPUT_HAS(input, INPUT_LEFT);
    if (!holding_back) return 0;

    /* Determine block stance from input */
    int crouching = INPUT_HAS(input, INPUT_DOWN);

    /* Low attacks beat standing block */
    if (hit_type == HIT_TYPE_LOW && !crouching) return 0;

    /* Overheads beat crouching block */
    if (hit_type == HIT_TYPE_OVERHEAD && crouching) return 0;

    /* MID / HIGH — blocked by either stance */
    return 1;
}

void hitbox_resolve_hit(CharacterState *attacker, CharacterState *defender,
                        const struct MoveData *move, int is_blocking,
                        ComboState *attacker_combo, ComboState *defender_combo,
                        int *hitstop, int counter_hit) {
    if (is_blocking) {
        /* Block: reduced damage, blockstun */
        int chip = move->chip_damage;
        if (chip < 1) chip = 1;  /* Minimum 1 chip */
        defender->hp -= chip;
        defender->blockstun_remaining = move->blockstun;
        defender->state = STATE_BLOCKSTUN;
        defender->state_frame = 0;

        /* Reduced knockback on block */
        defender->vx = move->knockback_x / 2 * (defender->x > attacker->x ? 1 : -1);

        /* Block resets combo counter */
        if (attacker_combo) {
            combo_on_block(attacker_combo);
        }
        if (defender_combo) {
            combo_reset(defender_combo);
        }
    } else {
        /* Hit: apply damage scaling */
        int damage = move->damage;
        int hitstun_bonus = 0; /* extra hitstun from counter hit */
        if (counter_hit) {
            damage = (damage * 110) / 100;   /* 1.1x damage */
            hitstun_bonus = 1; /* flag for later */
        }
        if (attacker_combo) {
            damage = combo_get_scaled_damage(attacker_combo, damage);
            /* Check if this is a light starter (5L or 2L) */
            int is_light = (move->damage == 1000 || move->damage == 800);
            combo_on_hit(attacker_combo, move->damage, is_light);
        }

        defender->hp -= damage;

        /* Apply hitstun with decay */
        int hitstun = move->hitstun;
        if (hitstun_bonus) {
            hitstun = (hitstun * 120) / 100; /* 1.2x hitstun on counter hit */
        }
        if (attacker_combo) {
            int decay = combo_get_hitstun_decay(attacker_combo);
            hitstun = (hitstun * decay) / 100;
            if (hitstun < 1) hitstun = 1;
        }

        defender->hitstun_remaining = hitstun;
        defender->in_hitstun = TRUE;
        defender->hit_flash = 3;  /* White flash for 3 frames */

        /* Knockdown or hitstun based on move properties */
        if (move->properties & (MOVE_PROP_HARD_KD | MOVE_PROP_SLIDING_KD)) {
            defender->state = STATE_KNOCKDOWN;
        } else {
            defender->state = STATE_HITSTUN;
        }
        defender->state_frame = 0;
        /* Note: don't uncrouch — defender stays at current height/position.
         * Crouching flag preserved so hitstun exit returns to crouch. */

        /* Knockback and airborne handling */
        if (!defender->on_ground) {
            /* Already airborne (juggle): slam them down into knockdown.
             * Preserves horizontal knockback but forces downward velocity. */
            defender->vx = move->knockback_x * (defender->x > attacker->x ? 1 : -1);
            defender->vy = FIXED_FROM_INT(8);  /* Fast downward slam */
            defender->state = STATE_HITSTUN;
            /* They'll land into knockdown via update_hitstun's landing check */
        } else if (move->properties & MOVE_PROP_LAUNCHER) {
            /* Launcher: send opponent airborne */
            defender->vx = move->knockback_x * (defender->x > attacker->x ? 1 : -1);
            defender->vy = move->knockback_y;
            defender->on_ground = FALSE;
            defender->state = STATE_HITSTUN;
            /* Uncrouch if crouching so airborne height is correct */
            if (defender->crouching) {
                defender->y -= FIXED_FROM_INT(defender->standing_height - defender->crouch_height);
                defender->height = defender->standing_height;
                defender->crouching = FALSE;
            }
        } else {
            /* Grounded hit: normal knockback */
            defender->vx = move->knockback_x * (defender->x > attacker->x ? 1 : -1);
            defender->vy = move->knockback_y;
        }
        if (move->properties & MOVE_PROP_WALL_BOUNCE) {
            /* Check if wall bounce is available and defender is near wall */
            if (attacker_combo && combo_can_wall_bounce(attacker_combo)) {
                int defender_x = FIXED_TO_INT(defender->x);
                /* Near left or right wall? */
                if (defender_x < 50 || defender_x > STAGE_WIDTH - 50 - defender->width) {
                    /* Apply wall bounce: reverse horizontal velocity, add upward bounce */
                    defender->vx = -defender->vx * 3 / 4;  /* Bounce back with 75% velocity */
                    defender->vy = FIXED_FROM_INT(-8);     /* Upward bounce */
                    defender->on_ground = FALSE;
                    defender->state = STATE_HITSTUN;       /* Stay in hitstun during bounce */
                    combo_use_wall_bounce(attacker_combo);
                }
            }
        }
        if (move->properties & MOVE_PROP_GROUND_BOUNCE) {
            /* Mark defender for ground bounce on landing */
            defender->ground_bounce_pending = TRUE;
        }
    }

    /* Apply hitstop scaled by damage (light=3, medium=5, heavy=7) */
    int hs = 3;
    if (move->damage >= 1500) hs = 5;
    if (move->damage >= 2000) hs = 7;
    /* Reduce hitstop for airborne attacker (no freeze-in-air jank) */
    if (!attacker->on_ground) hs = (hs + 1) / 2;
    *hitstop = hs;
}

void hitbox_apply_hitstop(int *hitstop, int frames) {
    *hitstop = frames;
}

void hitbox_update_hitstop(int *hitstop) {
    if (*hitstop > 0) {
        (*hitstop)--;
    }
}

int hitbox_in_hitstop(const int *hitstop) {
    return *hitstop > 0;
}
