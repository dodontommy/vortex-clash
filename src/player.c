#include "player.h"
#include "physics.h"
#include "hitbox.h"
#ifndef TESTING_HEADLESS
#include <raylib.h>
#endif
#include <string.h>
#include <stdio.h>

/* Constants */
#define WALK_FORWARD_SPEED  FIXED_FROM_INT(4)
#define WALK_BACKWARD_SPEED FIXED_FROM_INT(3)
#define JUMP_SQUAT_FRAMES   4
#define DASH_SPEED          FIXED_FROM_INT(14)
#define DASH_FORWARD_FRAMES 12
#define DASH_BACKWARD_FRAMES 14
#define LANDING_FRAMES      2
#define DOUBLE_TAP_FRAMES   10
#define DASH_CROUCH_CANCEL_FRAMES 6

void player_init(PlayerState *p, int player_id, int start_x, int start_y) {
    memset(p, 0, sizeof(PlayerState));
    p->player_id = player_id;
    p->active_char = 0;
    p->attack_hit_id = 0;
    p->opponent_hits[0] = -1;
    p->opponent_hits[1] = -1;

    CharacterState *c = &p->chars[0];
    c->state = STATE_IDLE;
    c->state_frame = 0;
    c->x = FIXED_FROM_INT(start_x);
    c->y = FIXED_FROM_INT(start_y);
    c->vx = 0;
    c->vy = 0;
    c->width = 50;
    c->standing_height = 80;
    c->crouch_height = 40;
    c->height = c->standing_height;
    c->color_r = (player_id == 1) ? 255 : 80;
    c->color_g = (player_id == 1) ? 80 : 255;
    c->color_b = 80;
    c->facing = (player_id == 1) ? 1 : -1;
    c->on_ground = TRUE;
    c->jumping = FALSE;
    c->dashing = FALSE;
    c->crouching = FALSE;
    c->blocking = FALSE;
    c->in_hitstun = FALSE;
    c->in_blockstun = FALSE;
    c->hp = 10000;
    c->max_hp = 10000;
    c->hitstun_remaining = 0;
    c->blockstun_remaining = 0;
}

static void update_idle(CharacterState *c, uint32_t input) {
    c->vx = 0;
    /* Jump takes priority over walk — fighting game standard */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->crouch_height;
        c->crouching = TRUE;
    } else if (INPUT_HAS(input, INPUT_RIGHT) || INPUT_HAS(input, INPUT_LEFT)) {
        int input_dir = INPUT_HAS(input, INPUT_RIGHT) ? 1 : -1;
        if (input_dir == c->facing) {
            c->state = STATE_WALK_FORWARD;
            c->vx = WALK_FORWARD_SPEED * c->facing;
        } else {
            c->state = STATE_WALK_BACKWARD;
            c->vx = -WALK_BACKWARD_SPEED * c->facing;
        }
        c->state_frame = 0;
    }
}

/* Check for double-tap and start dash if detected */
static void check_dash_input(PlayerState *p, CharacterState *c, uint32_t input) {
    /* Only detect on rising edge (newly pressed this frame) */
    uint32_t pressed = input & ~p->prev_input;
    int new_dir = 0;
    if (pressed & INPUT_RIGHT) new_dir = 1;
    else if (pressed & INPUT_LEFT) new_dir = -1;

    if (new_dir == 0) return;  /* No new direction press this frame */

    if (p->last_input_dir == new_dir) {
        int frames_since_last = p->frame_counter - p->dir_change_frame;
        if (frames_since_last <= DOUBLE_TAP_FRAMES) {
            if (new_dir == c->facing) {
                c->state = STATE_DASH_FORWARD;
            } else {
                c->state = STATE_DASH_BACKWARD;
            }
            c->state_frame = 0;
            p->last_input_dir = 0;
            return;
        }
    }

    p->last_input_dir = new_dir;
    p->dir_change_frame = p->frame_counter;
}

static void update_walk_forward(CharacterState *c, uint32_t input) {
    c->vx = WALK_FORWARD_SPEED * c->facing;
    
    /* Check if still holding forward relative to facing */
    int input_dir = 0;
    if (INPUT_HAS(input, INPUT_RIGHT)) input_dir = 1;
    else if (INPUT_HAS(input, INPUT_LEFT)) input_dir = -1;
    
    /* Jump priority over walk */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    } else if (input_dir != c->facing) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->vx = 0;
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->crouch_height;
        c->crouching = TRUE;
    }

    /* Apply velocity */
    c->x += c->vx;
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }
}

static void update_walk_backward(CharacterState *c, uint32_t input) {
    c->vx = -WALK_BACKWARD_SPEED * c->facing;

    /* Check if still holding backward relative to facing */
    int input_dir = 0;
    if (INPUT_HAS(input, INPUT_RIGHT)) input_dir = 1;
    else if (INPUT_HAS(input, INPUT_LEFT)) input_dir = -1;

    /* Jump priority over walk */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    } else if (input_dir != -c->facing) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->crouch_height;
        c->crouching = TRUE;
    }

    /* Apply velocity */
    c->x += c->vx;
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }
}

static void update_crouch(CharacterState *c, uint32_t input) {
    c->vx = 0;
    if (!INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->y -= FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->standing_height;
        c->crouching = FALSE;
    }
}

/* Uncrouch helper for dash-from-crouch */
static void uncrouch(CharacterState *c) {
    c->y -= FIXED_FROM_INT(c->standing_height - c->crouch_height);
    c->height = c->standing_height;
    c->crouching = FALSE;
}

static void update_jump_squat(CharacterState *c, uint32_t input) {
    c->vy = 0;

    /* Maintain ground momentum during jump squat */
    if (INPUT_HAS(input, INPUT_RIGHT)) {
        c->vx = WALK_FORWARD_SPEED;
    } else if (INPUT_HAS(input, INPUT_LEFT)) {
        c->vx = -WALK_FORWARD_SPEED;
    } else {
        c->vx = 0;
    }

    c->x += c->vx;
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }

    c->state_frame++;
    if (c->state_frame >= JUMP_SQUAT_FRAMES) {
        c->state = STATE_AIRBORNE;
        c->state_frame = 0;
        c->on_ground = FALSE;
        c->jumping = TRUE;
        c->vy = JUMP_VELOCITY;

        /* Carry horizontal momentum into air */
        if (INPUT_HAS(input, INPUT_RIGHT)) {
            c->vx = WALK_FORWARD_SPEED;
        } else if (INPUT_HAS(input, INPUT_LEFT)) {
            c->vx = -WALK_FORWARD_SPEED;
        } else {
            c->vx = 0;
        }
    }
}

static void update_airborne(CharacterState *c, uint32_t input) {
    c->vy += GRAVITY;
    c->x += c->vx;
    c->y += c->vy;
    
    int ground_top = GROUND_Y - c->height;
    if (c->y >= FIXED_FROM_INT(ground_top)) {
        c->y = FIXED_FROM_INT(ground_top);
        c->vy = 0;
        c->on_ground = TRUE;
        c->jumping = FALSE;
        c->state = STATE_LANDING;
        c->state_frame = 0;
    }
    
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }
}

static void update_landing(CharacterState *c, uint32_t input) {
    c->vx = 0;
    c->vy = 0;
    c->state_frame++;
    if (c->state_frame >= LANDING_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
    }
}

static void update_dash_forward(CharacterState *c, uint32_t input) {
    c->vx = DASH_SPEED * c->facing;
    c->dashing = TRUE;
    c->state_frame++;
    /* Crouch cancel after minimum frames */
    if (c->state_frame >= DASH_CROUCH_CANCEL_FRAMES && INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->dashing = FALSE;
        c->vx = 0;
        c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->crouch_height;
        c->crouching = TRUE;
        return;
    }
    if (c->state_frame >= DASH_FORWARD_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->dashing = FALSE;
    }
    /* Apply velocity */
    c->x += c->vx;
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }
}

static void update_dash_backward(CharacterState *c, uint32_t input) {
    c->vx = -DASH_SPEED * c->facing;
    c->dashing = TRUE;
    c->state_frame++;
    /* Crouch cancel after minimum frames */
    if (c->state_frame >= DASH_CROUCH_CANCEL_FRAMES && INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->dashing = FALSE;
        c->vx = 0;
        c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->crouch_height;
        c->crouching = TRUE;
        return;
    }
    if (c->state_frame >= DASH_BACKWARD_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->dashing = FALSE;
    }
    /* Apply velocity */
    c->x += c->vx;
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
        c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
    }
}

static void update_attack_startup(CharacterState *c, const AttackMove *move) {
    c->state_frame++;
    if (c->state_frame >= move->startup_frames) {
        c->state = STATE_ATTACK_ACTIVE;
        c->state_frame = 0;
    }
}

static void update_attack_active(CharacterState *c, const AttackMove *move) {
    c->state_frame++;
    int active_frames = move->active_end - move->active_start + 1;
    if (c->state_frame >= active_frames) {
        c->state = STATE_ATTACK_RECOVERY;
        c->state_frame = 0;
    }
}

static void update_attack_recovery(CharacterState *c, const AttackMove *move) {
    c->state_frame++;
    if (c->state_frame >= move->recovery_frames) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
    }
}

static void update_hitstun(CharacterState *c) {
    c->vx = 0;
    c->vy = 0;
    c->state_frame++;
    if (c->state_frame >= c->hitstun_remaining) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->in_hitstun = FALSE;
    }
}

static void update_blockstun(CharacterState *c) {
    c->vx = 0;
    c->vy = 0;
    c->state_frame++;
    if (c->state_frame >= c->blockstun_remaining) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->in_blockstun = FALSE;
    }
}

void player_update(PlayerState *p, uint32_t input) {
    CharacterState *c = &p->chars[p->active_char];
    p->frame_counter++;
    
    /* Check for dash from crouch (wavedash) */
    if (c->state == STATE_CROUCH) {
        CharacterStateEnum before = c->state;
        check_dash_input(p, c, input);
        if (c->state == STATE_DASH_FORWARD || c->state == STATE_DASH_BACKWARD) {
            uncrouch(c);
        }
    }

    /* Check for attack input in actionable states */
    if (c->state == STATE_IDLE || c->state == STATE_WALK_FORWARD || c->state == STATE_WALK_BACKWARD) {
        /* Check for attack button press */
        uint32_t pressed = input & ~p->prev_input;
        if (pressed & INPUT_LIGHT) {
            p->current_attack = &ATTACK_5L;
            p->attack_hit_id++;
            p->opponent_hits[0] = -1;
            p->opponent_hits[1] = -1;
            c->state = STATE_ATTACK_STARTUP;
            c->state_frame = 0;
        } else if (pressed & INPUT_MEDIUM) {
            p->current_attack = &ATTACK_5M;
            p->attack_hit_id++;
            p->opponent_hits[0] = -1;
            p->opponent_hits[1] = -1;
            c->state = STATE_ATTACK_STARTUP;
            c->state_frame = 0;
        } else if (pressed & INPUT_HEAVY) {
            p->current_attack = &ATTACK_5H;
            p->attack_hit_id++;
            p->opponent_hits[0] = -1;
            p->opponent_hits[1] = -1;
            c->state = STATE_ATTACK_STARTUP;
            c->state_frame = 0;
        } else {
            check_dash_input(p, c, input);
        }
    }
    
    switch (c->state) {
        case STATE_IDLE: update_idle(c, input); break;
        case STATE_WALK_FORWARD: update_walk_forward(c, input); break;
        case STATE_WALK_BACKWARD: update_walk_backward(c, input); break;
        case STATE_CROUCH: update_crouch(c, input); break;
        case STATE_JUMP_SQUAT: update_jump_squat(c, input); break;
        case STATE_AIRBORNE: update_airborne(c, input); break;
        case STATE_LANDING: update_landing(c, input); break;
        case STATE_DASH_FORWARD: update_dash_forward(c, input); break;
        case STATE_DASH_BACKWARD: update_dash_backward(c, input); break;
        case STATE_ATTACK_STARTUP: 
            if (p->current_attack) update_attack_startup(c, p->current_attack); 
            break;
        case STATE_ATTACK_ACTIVE: 
            if (p->current_attack) update_attack_active(c, p->current_attack); 
            break;
        case STATE_ATTACK_RECOVERY: 
            if (p->current_attack) update_attack_recovery(c, p->current_attack); 
            break;
        case STATE_HITSTUN: update_hitstun(c); break;
        case STATE_BLOCKSTUN: update_blockstun(c); break;
        default: break;
    }

    p->prev_input = input;
}

void player_update_facing(PlayerState *p1, PlayerState *p2) {
    CharacterState *c1 = &p1->chars[p1->active_char];
    CharacterState *c2 = &p2->chars[p2->active_char];
    
    if (c1->on_ground && c2->on_ground &&
        !c1->in_hitstun && !c2->in_hitstun &&
        !c1->dashing && !c2->dashing &&
        c1->state != STATE_JUMP_SQUAT && c2->state != STATE_JUMP_SQUAT) {
        
        fixed_t p1_center = c1->x + FIXED_FROM_INT(c1->width / 2);
        fixed_t p2_center = c2->x + FIXED_FROM_INT(c2->width / 2);
        
        if (p1_center < p2_center) {
            c1->facing = 1;
            c2->facing = -1;
        } else {
            c1->facing = -1;
            c2->facing = 1;
        }
    }
}

void player_resolve_collisions(PlayerState *p1, PlayerState *p2) {
    CharacterState *c1 = &p1->chars[p1->active_char];
    CharacterState *c2 = &p2->chars[p2->active_char];
    
    /* Simple AABB collision */
    int c1_left = FIXED_TO_INT(c1->x);
    int c1_right = c1_left + c1->width;
    int c2_left = FIXED_TO_INT(c2->x);
    int c2_right = c2_left + c2->width;
    
    /* Check for overlap */
    if (c1_left < c2_right && c1_right > c2_left) {
        /* Push them apart */
        int overlap = (c1_right - c2_left < c2_right - c1_left) ? 
                      (c1_right - c2_left) : (c2_right - c1_left);
        int push = overlap / 2 + 1;  /* +1 to ensure separation */
        
        if (c1->x < c2->x) {
            /* P1 is on left, push left */
            c1->x -= FIXED_FROM_INT(push);
            c2->x += FIXED_FROM_INT(push);
        } else {
            c1->x += FIXED_FROM_INT(push);
            c2->x -= FIXED_FROM_INT(push);
        }
        
        /* Re-apply stage bounds */
        if (c1->x < 0) c1->x = 0;
        if (c1->x > FIXED_FROM_INT(SCREEN_WIDTH - c1->width)) {
            c1->x = FIXED_FROM_INT(SCREEN_WIDTH - c1->width);
        }
        if (c2->x < 0) c2->x = 0;
        if (c2->x > FIXED_FROM_INT(SCREEN_WIDTH - c2->width)) {
            c2->x = FIXED_FROM_INT(SCREEN_WIDTH - c2->width);
        }
    }
}

static const char *state_to_string(CharacterStateEnum state) {
    switch (state) {
        case STATE_IDLE: return "IDLE";
        case STATE_WALK_FORWARD: return "WALK_FWD";
        case STATE_WALK_BACKWARD: return "WALK_BACK";
        case STATE_CROUCH: return "CROUCH";
        case STATE_JUMP_SQUAT: return "JUMP_SQUAT";
        case STATE_AIRBORNE: return "AIRBORNE";
        case STATE_LANDING: return "LANDING";
        case STATE_DASH_FORWARD: return "DASH_FWD";
        case STATE_DASH_BACKWARD: return "DASH_BACK";
        default: return "UNKNOWN";
    }
}

#ifndef TESTING_HEADLESS
void player_render(const PlayerState *p) {
    const CharacterState *c = &p->chars[p->active_char];
    int x = FIXED_TO_INT(c->x);
    int y = FIXED_TO_INT(c->y);
    Color color = (Color){c->color_r, c->color_g, c->color_b, 255};
    DrawRectangle(x, y, c->width, c->height, color);

    /* Debug: state name */
    DrawText(state_to_string(c->state), x, y - 20, 12, (Color){200, 200, 200, 255});

    /* Debug: player number */
    char pnum[8];
    snprintf(pnum, sizeof(pnum), "P%d", p->player_id);
    DrawText(pnum, x, y + c->height + 2, 12, (Color){200, 200, 200, 255});
}
#endif
