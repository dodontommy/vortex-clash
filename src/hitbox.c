#include "hitbox.h"
#include "character.h"
#include "combo.h"
#include "feel.h"
#include "sprite.h"
#include <string.h>

static int impact_hitstop_for_move(const MoveData *move, int is_blocking, int counter_hit) {
    int hs = HITSTOP_NORMAL_M;

    if (move->move_type == MOVE_TYPE_NORMAL) {
        if (move->strength <= 0) hs = HITSTOP_NORMAL_L;
        else if (move->strength == 1) hs = HITSTOP_NORMAL_M;
        else hs = HITSTOP_NORMAL_H;
    } else if (move->move_type == MOVE_TYPE_SPECIAL) {
        if (move->strength <= 0) hs = HITSTOP_SPECIAL_L;
        else if (move->strength == 1) hs = HITSTOP_SPECIAL_M;
        else hs = HITSTOP_SPECIAL_H;
    } else if (move->move_type == MOVE_TYPE_SUPER) {
        hs = HITSTOP_SUPER;
    } else if (move->move_type == MOVE_TYPE_THROW) {
        hs = HITSTOP_THROW;
    }

    if (move->properties & MOVE_PROP_LAUNCHER) hs += HITSTOP_LAUNCHER_BONUS;
    if (move->properties & MOVE_PROP_WALL_BOUNCE) hs += HITSTOP_WALLBOUNCE_BONUS;
    if (counter_hit) hs += HITSTOP_COUNTER_BONUS;

    if (is_blocking) {
        hs -= (hs >= 8) ? HITSTOP_BLOCK_REDUCE_HI : HITSTOP_BLOCK_REDUCE_LO;
        if (hs < HITSTOP_BLOCK_FLOOR) hs = HITSTOP_BLOCK_FLOOR;
    }

    if (hs > HITSTOP_CAP) hs = HITSTOP_CAP;
    return hs;
}

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
    int direction = (defender->x > attacker->x) ? 1 : -1;
    int combo_hits_before = attacker_combo ? attacker_combo->hit_count : 0;

    if (is_blocking) {
        /* Block: reduced damage, blockstun */
        int chip = move->chip_damage;
        if (chip < 1) chip = 1;  /* Minimum 1 chip */
        /* Chip cannot KO: defender is left at 1 HP minimum. */
        if (defender->hp > 1) {
            defender->hp -= chip;
            if (defender->hp < 1) defender->hp = 1;
        }
        defender->blockstun_remaining = move->blockstun;
        defender->state = STATE_BLOCKSTUN;
        defender->state_frame = 0;
        defender->current_anim = ANIM_BLOCKSTUN;
        defender->anim_frame = defender->crouching ? 2 : 0;
        defender->anim_timer = 0;
        defender->throw_timer = 0;
        defender->throw_owner_player = -1;
        defender->throw_damage_applied = FALSE;
        defender->impact_pop_frames = 0;
        defender->impact_pop_vy = 0;
        defender->impact_pop_return = 0;

        /* Block pushback is greater than hit pushback (creates distance/safety) */
        defender->vx = move->knockback_x * 130 / 100 * direction;

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
        int is_light_hit = (move->move_type == MOVE_TYPE_NORMAL && move->strength == 0);
        int hitstun_bonus = 0; /* extra hitstun from counter hit */
        if (counter_hit) {
            damage = (damage * CH_DAMAGE_MULT) / 100;   /* 1.3x damage */
            hitstun_bonus = 1; /* flag for later */
        }
        if (attacker_combo) {
            damage = combo_apply_hit(attacker_combo, damage, is_light_hit);
        }

        defender->hp -= damage;

        /* Blue health: portion of damage becomes recoverable off-screen */
        {
            int blue = (damage * BLUE_HEALTH_PERCENT) / 100;
            defender->blue_hp += blue;
            /* Cap so hp + blue_hp <= max_hp */
            if (defender->hp + defender->blue_hp > defender->max_hp) {
                defender->blue_hp = defender->max_hp - defender->hp;
                if (defender->blue_hp < 0) defender->blue_hp = 0;
            }
        }

        /* Apply hitstun with decay */
        int hitstun = move->hitstun;
        int pushback_scale = 100;
        int impact_scale = 108;

        if (move->strength >= 1 || move->damage >= 1500) {
            hitstun += 1;
            impact_scale += 8;
        }
        if (move->strength >= 2 || move->damage >= 2000) {
            hitstun += 1;
            impact_scale += 14;
        }
        /* Keep chained confirms stable across spacing by soft-capping follow-up pushback. */
        if (combo_hits_before >= 2) pushback_scale = 90;
        else if (combo_hits_before >= 1) pushback_scale = 94;
        impact_scale = (impact_scale * pushback_scale) / 100;
        if (impact_scale < 70) impact_scale = 70;

        if (hitstun_bonus) {
            hitstun = (hitstun * CH_HITSTUN_MULT) / 100; /* 1.3x hitstun on counter hit */
        }
        if (attacker_combo) {
            int decay = combo_get_hitstun_decay(attacker_combo);
            if (!defender->on_ground) {
                decay = 100 - (100 - decay) / 2;  /* Half penalty in air */
            }
            hitstun = (hitstun * decay) / 100;
            if (!defender->on_ground && hitstun < JUGGLE_HITSTUN_FLOOR)
                hitstun = JUGGLE_HITSTUN_FLOOR;
            if (hitstun < 1) hitstun = 1;
        }

        defender->hitstun_remaining = hitstun;
        defender->in_hitstun = TRUE;
        defender->hit_flash = HIT_FLASH_FRAMES;
        defender->current_anim = ANIM_HITSTUN;
        defender->anim_frame = !defender->on_ground ? 4 : (defender->crouching ? 3 : 1);
        defender->anim_timer = 0;
        defender->throw_timer = 0;
        defender->throw_owner_player = -1;
        defender->throw_damage_applied = FALSE;
        defender->impact_pop_frames = 0;
        defender->impact_pop_vy = 0;
        defender->impact_pop_return = 0;

        /* Default to hitstun; knockdown override applied after velocity block */
        defender->state = STATE_HITSTUN;
        defender->state_frame = 0;
        /* Note: don't uncrouch — defender stays at current height/position.
         * Crouching flag preserved so hitstun exit returns to crouch. */

        /* Knockback and airborne handling */
        if (!defender->on_ground) {
            /* Already airborne (juggle): use move's actual knockback_y
             * plus progressive juggle gravity instead of hardcoded slam. */
            defender->vx = move->knockback_x * impact_scale / 100 * direction;
            int juggle_hits = attacker_combo ? attacker_combo->juggle_count : 0;
            fixed_t juggle_pull = (fixed_t)juggle_hits * JUGGLE_GRAVITY_PER_HIT;
            fixed_t base_vy = move->knockback_y;
            /* Minimum upward bounce for non-KD air hits to sustain combos.
             * Without this, weak knockback_y (-2, -3) gets eaten by gravity
             * in 4-6 frames and the opponent plummets immediately. */
            if (!(move->properties & (MOVE_PROP_HARD_KD | MOVE_PROP_SLIDING_KD))) {
                if (base_vy > JUGGLE_MIN_VY) base_vy = JUGGLE_MIN_VY;
            }
            defender->vy = base_vy + juggle_pull;
            if (attacker_combo) attacker_combo->juggle_count++;
        } else if (move->properties & MOVE_PROP_LAUNCHER) {
            /* Launcher: send opponent airborne */
            defender->vx = move->knockback_x * impact_scale / 100 * direction;
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
            defender->vx = move->knockback_x * impact_scale / 100 * direction;
            defender->vy = move->knockback_y;
            /* Visual weight: brief lift on grounded impacts before pushback slide. */
            if (defender->on_ground) {
                int pop_px = IMPACT_POP_L;
                if (move->strength == 1) pop_px = IMPACT_POP_M;
                if (move->strength >= 2 || move->move_type >= MOVE_TYPE_SPECIAL) pop_px = IMPACT_POP_H;
                defender->impact_pop_frames = 1;
                defender->impact_pop_vy = FIXED_FROM_INT(-pop_px);
                defender->impact_pop_return = FIXED_FROM_INT(pop_px);
            }
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
        /* After all velocity/state assignment: knockdown override wins */
        if (move->properties & (MOVE_PROP_HARD_KD | MOVE_PROP_SLIDING_KD)) {
            defender->state = STATE_KNOCKDOWN;
        }
    }

    /* Tuned to keep impact while avoiding slow-motion feel in strings. */
    *hitstop = impact_hitstop_for_move(move, is_blocking, counter_hit);
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

int hitbox_consume_hitstop(int *hitstop) {
    if (*hitstop > 0) {
        (*hitstop)--;
        return 1;
    }
    return 0;
}
