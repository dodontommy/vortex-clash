#include "player.h"
#include "physics.h"
#include <raylib.h>
#include <string.h>
#include <stdio.h>

/* Constants */
#define WALK_FORWARD_SPEED  FIXED_FROM_INT(4)
#define WALK_BACKWARD_SPEED FIXED_FROM_INT(3)
#define JUMP_SQUAT_FRAMES   4
#define DASH_SPEED          FIXED_FROM_INT(6)
#define DASH_FORWARD_FRAMES 18
#define DASH_BACKWARD_FRAMES 22
#define LANDING_FRAMES      2
#define DOUBLE_TAP_FRAMES   10  /* Max frames between taps for dash */

void player_init(PlayerState *p, int player_id, int start_x, int start_y) {
    memset(p, 0, sizeof(PlayerState));
    p->player_id = player_id;
    p->active_char = 0;

    CharacterState *c = &p->chars[0];
    c->state = STATE_IDLE;
    c->state_frame = 0;
    c->x = FIXED_FROM_INT(start_x);
    c->y = FIXED_FROM_INT(start_y);
    c->vx = 0;
    c->vy = 0;
    c->width = 50;
    c->height = 80;
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
}

static void update_idle(CharacterState *c, uint32_t input) {
    c->vx = 0;
    if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->height = 40;
        c->crouching = TRUE;
    } else if (INPUT_HAS(input, INPUT_RIGHT)) {
        c->state = STATE_WALK_FORWARD;
        c->state_frame = 0;
        c->vx = WALK_FORWARD_SPEED;
    } else if (INPUT_HAS(input, INPUT_LEFT)) {
        c->state = STATE_WALK_BACKWARD;
        c->state_frame = 0;
        c->vx = -WALK_BACKWARD_SPEED;
    } else if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    }
}

/* Check for double-tap and start dash if detected */
static void check_dash_input(CharacterState *c, uint32_t input, int *last_dir, int *last_dir_frame, int current_frame) {
    int new_dir = 0;
    if (INPUT_HAS(input, INPUT_RIGHT)) new_dir = 1;
    else if (INPUT_HAS(input, INPUT_LEFT)) new_dir = -1;
    
    if (new_dir != 0 && *last_dir == new_dir) {
        /* Double tap detected! */
        int frames_since_last = current_frame - *last_dir_frame;
        if (frames_since_last <= DOUBLE_TAP_FRAMES) {
            /* Start dash */
            if (new_dir == c->facing) {
                c->state = STATE_DASH_FORWARD;
            } else {
                c->state = STATE_DASH_BACKWARD;
            }
            c->state_frame = 0;
            *last_dir = 0;  /* Reset to prevent continuous dashes */
            return;
        }
    }
    
    if (new_dir != 0) {
        *last_dir = new_dir;
        *last_dir_frame = current_frame;
    }
}

static void update_walk_forward(CharacterState *c, uint32_t input) {
    c->vx = WALK_FORWARD_SPEED;
    if (!INPUT_HAS(input, INPUT_RIGHT)) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->height = 40;
    } else if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    }
}

static void update_walk_backward(CharacterState *c, uint32_t input) {
    c->vx = -WALK_BACKWARD_SPEED;
    if (!INPUT_HAS(input, INPUT_LEFT)) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->height = 40;
    } else if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
    }
}

static void update_crouch(CharacterState *c, uint32_t input) {
    c->vx = 0;
    if (!INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->height = 80;
        c->crouching = FALSE;
    }
}

static void update_jump_squat(CharacterState *c, uint32_t input) {
    c->vx = 0;
    c->vy = 0;
    c->state_frame++;
    if (c->state_frame >= JUMP_SQUAT_FRAMES) {
        c->state = STATE_AIRBORNE;
        c->state_frame = 0;
        c->on_ground = FALSE;
        c->jumping = TRUE;
        c->vy = JUMP_VELOCITY;
        if (INPUT_HAS(input, INPUT_RIGHT) && INPUT_HAS(input, INPUT_UP)) {
            c->vx = WALK_FORWARD_SPEED;
        } else if (INPUT_HAS(input, INPUT_LEFT) && INPUT_HAS(input, INPUT_UP)) {
            c->vx = -WALK_FORWARD_SPEED;
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
    if (c->state_frame >= DASH_FORWARD_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->dashing = FALSE;
    }
}

static void update_dash_backward(CharacterState *c, uint32_t input) {
    c->vx = -DASH_SPEED * c->facing;
    c->dashing = TRUE;
    c->state_frame++;
    if (c->state_frame >= DASH_BACKWARD_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->dashing = FALSE;
    }
}

/* Update facing direction - characters always face each other */
static void update_facing(CharacterState *p1, CharacterState *p2) {
    if (p1->on_ground && p2->on_ground &&
        !p1->in_hitstun && !p2->in_hitstun &&
        !p1->dashing && !p2->dashing) {
        
        fixed_t p1_center = p1->x + FIXED_FROM_INT(p1->width / 2);
        fixed_t p2_center = p2->x + FIXED_FROM_INT(p2->width / 2);
        
        if (p1_center < p2_center) {
            /* P1 should face right, P2 should face left */
            p1->facing = 1;
            p2->facing = -1;
        } else {
            /* P1 should face left, P2 should face right */
            p1->facing = -1;
            p2->facing = 1;
        }
    }
}

void player_update(PlayerState *p, uint32_t input) {
    CharacterState *c = &p->chars[p->active_char];
    p->frame_counter++;
    
    /* Check for dash input in idle or walk states */
    if (c->state == STATE_IDLE || c->state == STATE_WALK_FORWARD || c->state == STATE_WALK_BACKWARD) {
        check_dash_input(c, input, &p->last_input_dir, &p->dir_change_frame, p->frame_counter);
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
        default: break;
    }
    if (c->state != STATE_AIRBORNE) {
        c->x += c->vx;
        if (c->x < 0) c->x = 0;
        if (c->x > FIXED_FROM_INT(SCREEN_WIDTH - c->width)) {
            c->x = FIXED_FROM_INT(SCREEN_WIDTH - c->width);
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