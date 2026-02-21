#include "hitbox.h"
#include <string.h>

/* Global hitstop */
int g_hitstop_frames = 0;

/* Pre-defined attacks */
const AttackMove ATTACK_5L = {
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 10,
    .damage = 1000,
    .hitstun = 12,
    .blockstun = 8,
    .chip_damage = 100,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = 0,
    .x_offset = 50,
    .y_offset = 20,
    .width = 30,
    .height = 30
};

const AttackMove ATTACK_5M = {
    .startup_frames = 8,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 14,
    .damage = 1500,
    .hitstun = 15,
    .blockstun = 10,
    .chip_damage = 150,
    .knockback_x = FIXED_FROM_INT(4),
    .knockback_y = 0,
    .x_offset = 55,
    .y_offset = 15,
    .width = 40,
    .height = 35
};

const AttackMove ATTACK_5H = {
    .startup_frames = 12,
    .active_start = 0,
    .active_end = 3,
    .recovery_frames = 20,
    .damage = 2000,
    .hitstun = 20,
    .blockstun = 14,
    .chip_damage = 200,
    .knockback_x = FIXED_FROM_INT(6),
    .knockback_y = FIXED_FROM_INT(-2),
    .x_offset = 60,
    .y_offset = 10,
    .width = 50,
    .height = 40
};

void hitbox_init(void) {
    g_hitstop_frames = 0;
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

/* Check if defender is blocking (holding back relative to attacker) */
int is_blocking(CharacterState *defender, CharacterState *attacker, uint32_t input) {
    /* Defender must be holding back relative to attacker */
    if (attacker->x < defender->x) {
        /* Attacker is to the left, defender must hold left */
        return INPUT_HAS(input, INPUT_LEFT) ? 1 : 0;
    } else {
        /* Attacker is to the right, defender must hold right */
        return INPUT_HAS(input, INPUT_RIGHT) ? 1 : 0;
    }
}

void hitbox_resolve_hit(CharacterState *attacker, CharacterState *defender, const AttackMove *move, int is_blocking) {
    if (is_blocking) {
        /* Block: reduced damage, blockstun */
        defender->hp -= move->chip_damage;
        defender->blockstun_remaining = move->blockstun;
        defender->state = STATE_BLOCKSTUN;
        defender->state_frame = 0;
        
        /* Reduced knockback on block */
        defender->vx = move->knockback_x / 2 * (defender->x > attacker->x ? 1 : -1);
    } else {
        /* Hit: full damage, hitstun */
        defender->hp -= move->damage;
        defender->hitstun_remaining = move->hitstun;
        defender->state = STATE_HITSTUN;
        defender->state_frame = 0;
        
        /* Full knockback */
        defender->vx = move->knockback_x * (defender->x > attacker->x ? 1 : -1);
        defender->vy = move->knockback_y;
    }
    
    /* Apply hitstop */
    g_hitstop_frames = 8;
}

/* Pushbox collision - prevent characters from overlapping */
void pushbox_resolve(CharacterState *c1, CharacterState *c2) {
    int c1_left = FIXED_TO_INT(c1->x);
    int c1_right = c1_left + c1->width;
    int c2_left = FIXED_TO_INT(c2->x);
    int c2_right = c2_left + c2->width;
    
    if (c1_left < c2_right && c1_right > c2_left) {
        int overlap = (c1_right - c2_left < c2_right - c1_left) ?
                      (c1_right - c2_left) : (c2_right - c1_left);
        int push = overlap / 2 + 1;
        
        if (c1->x < c2->x) {
            c1->x -= FIXED_FROM_INT(push);
            c2->x += FIXED_FROM_INT(push);
        } else {
            c1->x += FIXED_FROM_INT(push);
            c2->x -= FIXED_FROM_INT(push);
        }
        
        /* Stage bounds */
        if (c1->x < 0) c1->x = 0;
        if (c1->x > FIXED_FROM_INT(SCREEN_WIDTH - c1->width))
            c1->x = FIXED_FROM_INT(SCREEN_WIDTH - c1->width);
        if (c2->x < 0) c2->x = 0;
        if (c2->x > FIXED_FROM_INT(SCREEN_WIDTH - c2->width))
            c2->x = FIXED_FROM_INT(SCREEN_WIDTH - c2->width);
    }
}

void hitbox_apply_hitstop(int frames) {
    g_hitstop_frames = frames;
}

void hitbox_update_hitstop(void) {
    if (g_hitstop_frames > 0) {
        g_hitstop_frames--;
    }
}

int hitbox_in_hitstop(void) {
    return g_hitstop_frames > 0;
}