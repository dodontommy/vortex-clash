#include "player.h"
#include "sprite.h"
#include "hitbox.h"
#include "combo.h"
#include "character.h"
#ifndef TESTING_HEADLESS
#include <raylib.h>
#endif
#include <string.h>
#include <stdio.h>

/* Engine constants (same for all characters) */
#define DOUBLE_TAP_FRAMES   10
#define DASH_CROUCH_CANCEL_FRAMES 6
#define DASH_BUTTON_CANCEL_FRAMES 2  /* Min frames before attack button can cancel dash */
#define TWO_BUTTON_DASH_LENIENCY 5  /* Frames of leniency for two-button dash input */
#define COMBO_BUFFER_WINDOW      5  /* Frames to hold a buffered combo input */

/* Set animation on character state. Resets frame/timer if anim changed. */
static void set_anim(CharacterState *c, int anim_id) {
    if (c->current_anim == anim_id) return;
    c->current_anim = anim_id;
    c->anim_frame = 0;
    c->anim_timer = 0;
    /* Reset cached params so sprite_sync_anim() knows to refresh them */
    c->anim_frame_count = 0;
    c->anim_frame_duration = 1;
    c->anim_looping = 0;
}

/* Advance animation frame using cached params. No SpriteSet needed. */
void advance_anim(CharacterState *c) {
    if (c->anim_frame_count <= 0) return;
    c->anim_timer++;
    if (c->anim_timer >= c->anim_frame_duration) {
        c->anim_timer = 0;
        c->anim_frame++;
        if (c->anim_frame >= c->anim_frame_count) {
            c->anim_frame = c->anim_looping ? 0 : c->anim_frame_count - 1;
        }
    }
}

/* Get character definition for this player */
static const CharacterDef *get_char_def(const PlayerState *p) {
    return character_get_def((CharacterId)p->character_id);
}

/* --- Extracted helpers --- */

static int get_input_dir(uint32_t input) {
    if (INPUT_HAS(input, INPUT_RIGHT)) return 1;
    if (INPUT_HAS(input, INPUT_LEFT)) return -1;
    return 0;
}

static void clear_dash_buttons(PlayerState *p) {
    p->button_press_frame[0] = p->button_press_frame[1] = p->button_press_frame[2] = -100;
}

/* Check if two attack buttons were pressed within the leniency window.
 * Returns 1 if a dash should fire. Requires at least one newly pressed button
 * this frame, plus another button pressed within the last N frames. */
static int two_button_dash_input(const PlayerState *p, uint32_t pressed) {
    if (!(pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY))) return 0;
    int frame = p->frame_counter;
    int recent = 0;
    for (int i = 0; i < 3; i++) {
        if (frame - p->button_press_frame[i] < TWO_BUTTON_DASH_LENIENCY) recent++;
    }
    return recent >= 2;
}

/* Start a dash based on held direction (used by two-button dash and plink cancel) */
static void start_dash(CharacterState *c, uint32_t input) {
    if (!c->on_ground) return;  /* No air dashes (yet) */

    int input_dir = get_input_dir(input);

    if (input_dir == -c->facing) {
        c->state = STATE_DASH_BACKWARD;
        set_anim(c, ANIM_DASH_BACK);
    } else {
        c->state = STATE_DASH_FORWARD;
        set_anim(c, ANIM_DASH_FWD);
    }
    c->state_frame = 0;
    c->dashing = FALSE;
}

void player_init(PlayerState *p, int player_id, int start_x, int start_y, int char_id) {
    memset(p, 0, sizeof(PlayerState));
    p->player_id = player_id;
    p->character_id = char_id;
    p->active_char = 0;
    p->attack_hit_id = 0;
    p->opponent_hits[0] = -1;
    p->opponent_hits[1] = -1;
    clear_dash_buttons(p);
    p->pending_attack = 0;
    p->buffered_button = 0;
    p->buffered_button_frame = -100;
    p->last_down_frame = -100;

    /* Initialize combo state */
    combo_init(&p->combo);
    p->meter = 0;
    p->blue_hp = 0;

    const CharacterDef *def = character_get_def((CharacterId)char_id);

    CharacterState *c = &p->chars[0];
    c->state = STATE_IDLE;
    c->state_frame = 0;
    c->x = FIXED_FROM_INT(start_x);
    c->y = FIXED_FROM_INT(start_y);
    c->vx = 0;
    c->vy = 0;
    c->width = def->width;
    c->standing_height = def->standing_height;
    c->crouch_height = def->crouch_height;
    c->height = c->standing_height;
    c->color_r = (player_id == 1) ? 255 : 80;
    c->color_g = (player_id == 1) ? 80 : 255;
    c->color_b = 80;
    c->facing = (player_id == 1) ? 1 : -1;
    c->on_ground = TRUE;
    c->dashing = FALSE;
    c->crouching = FALSE;
    c->in_hitstun = FALSE;
    c->in_blockstun = FALSE;
    c->hp = def->max_hp;
    c->max_hp = def->max_hp;
    c->hitstun_remaining = 0;
    c->blockstun_remaining = 0;
    c->current_anim = ANIM_IDLE;
    c->anim_frame = 0;
    c->anim_timer = 0;
    c->anim_frame_duration = 1;
    c->anim_frame_count = 0;
    c->anim_looping = 1;
}

/* Crouch/uncrouch helpers */
static void uncrouch(CharacterState *c) {
    c->y -= FIXED_FROM_INT(c->standing_height - c->crouch_height);
    c->height = c->standing_height;
    c->crouching = FALSE;
}

static void enter_crouch(CharacterState *c) {
    c->y += FIXED_FROM_INT(c->standing_height - c->crouch_height);
    c->height = c->crouch_height;
    c->crouching = TRUE;
}

/* Clamp character position to stage boundaries */
static void clamp_stage_bounds(CharacterState *c) {
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(STAGE_WIDTH - c->width))
        c->x = FIXED_FROM_INT(STAGE_WIDTH - c->width);
}

static void update_idle(CharacterState *c, uint32_t input, const CharacterDef *def) {
    c->vx = 0;
    set_anim(c, ANIM_IDLE);
    /* Jump takes priority over walk — fighting game standard */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
        set_anim(c, ANIM_JUMP);
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        set_anim(c, ANIM_CROUCH);
        enter_crouch(c);
    } else {
        int dir = get_input_dir(input);
        if (dir == c->facing) {
            c->state = STATE_WALK_FORWARD;
            c->vx = def->walk_speed_forward * c->facing;
            set_anim(c, ANIM_WALK_FWD);
            c->state_frame = 0;
        } else if (dir == -c->facing) {
            c->state = STATE_WALK_BACKWARD;
            c->vx = -def->walk_speed_backward * c->facing;
            set_anim(c, ANIM_WALK_BACK);
            c->state_frame = 0;
        }
    }
}

/* Check for double-tap and start dash if detected */
static void check_dash_input(PlayerState *p, CharacterState *c, uint32_t input) {
    if (!c->on_ground) return;  /* No air dashes (yet) */

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
                set_anim(c, ANIM_DASH_FWD);
            } else {
                c->state = STATE_DASH_BACKWARD;
                set_anim(c, ANIM_DASH_BACK);
            }
            c->state_frame = 0;
            p->last_input_dir = 0;
            return;
        }
    }

    p->last_input_dir = new_dir;
    p->dir_change_frame = p->frame_counter;
}

/* Unified walk update — dir = 1 for forward, -1 for backward */
static void update_walk(CharacterState *c, uint32_t input, const CharacterDef *def, int dir) {
    if (dir == 1)
        c->vx = def->walk_speed_forward * c->facing;
    else
        c->vx = -def->walk_speed_backward * c->facing;

    int input_dir = get_input_dir(input);

    /* Jump priority over walk */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
        set_anim(c, ANIM_JUMP);
    } else if (input_dir != dir * c->facing) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->vx = 0;
        set_anim(c, ANIM_IDLE);
    } else if (INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        set_anim(c, ANIM_CROUCH);
        enter_crouch(c);
    }

    /* Apply velocity */
    c->x += c->vx;
    clamp_stage_bounds(c);
}

static void update_crouch(CharacterState *c, uint32_t input) {
    c->vx = 0;
    set_anim(c, ANIM_CROUCH);
    if (!INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        set_anim(c, ANIM_IDLE);
        c->y -= FIXED_FROM_INT(c->standing_height - c->crouch_height);
        c->height = c->standing_height;
        c->crouching = FALSE;
    }
}

/* Resolve which normal attack to use based on directional input.
 * During neutral (idle/walk/crouch), crouching flag matches input.
 * During chain cancels, the crouching flag may be stale from the
 * previous move — so we always use the live directional input. */
static const struct MoveData *resolve_normal(const PlayerState *p, uint32_t button, uint32_t input) {
    const CharacterState *c = &p->chars[p->active_char];
    int idx;
    /* S button: universal launcher (ground) or air exchange (air) */
    if (button & INPUT_SPECIAL) {
        idx = c->on_ground ? NORMAL_5S : NORMAL_JS;
        return character_get_normal(p->character_id, idx);
    }
    if (!c->on_ground) {
        /* Air command normals: j.2+button (dive kicks, etc.)
         * Falls back to regular air normal if slot is empty. */
        if (INPUT_HAS(input, INPUT_DOWN)) {
            idx = (button & INPUT_LIGHT) ? NORMAL_J2L : (button & INPUT_MEDIUM) ? NORMAL_J2M : NORMAL_J2H;
            const struct MoveData *move = character_get_normal(p->character_id, idx);
            if (move) return move;
        }
        idx = (button & INPUT_LIGHT) ? NORMAL_JL : (button & INPUT_MEDIUM) ? NORMAL_JM : NORMAL_JH;
    } else if (INPUT_HAS(input, INPUT_DOWN))
        idx = (button & INPUT_LIGHT) ? NORMAL_2L : (button & INPUT_MEDIUM) ? NORMAL_2M : NORMAL_2H;
    else
        idx = (button & INPUT_LIGHT) ? NORMAL_5L : (button & INPUT_MEDIUM) ? NORMAL_5M : NORMAL_5H;
    return character_get_normal(p->character_id, idx);
}

static void update_jump_squat(PlayerState *p, CharacterState *c, uint32_t input, const CharacterDef *def) {
    c->vy = 0;

    /* Maintain ground momentum during jump squat (dash momentum if dashing) */
    if (c->dashing) {
        /* Keep dash velocity — will carry into air */
    } else {
        int dir = get_input_dir(input);
        c->vx = dir * def->walk_speed_forward;
    }

    c->x += c->vx;
    clamp_stage_bounds(c);

    c->state_frame++;
    if (c->state_frame >= def->jump_squat_frames) {
        c->state = STATE_AIRBORNE;
        c->state_frame = 0;
        set_anim(c, ANIM_JUMP);
        c->on_ground = FALSE;
        c->vy = c->super_jump ? def->super_jump_velocity : def->jump_velocity;

        /* Carry horizontal momentum into air (dash momentum if dashing, capped) */
        if (c->dashing) {
            /* Cap dash-jump horizontal speed to prevent insane distance */
            if (c->vx > DASH_JUMP_MAX_SPEED) c->vx = DASH_JUMP_MAX_SPEED;
            else if (c->vx < -DASH_JUMP_MAX_SPEED) c->vx = -DASH_JUMP_MAX_SPEED;
            c->dashing = FALSE;
            c->dash_jump = TRUE;
        } else {
            int dir = get_input_dir(input);
            c->vx = dir * def->walk_speed_forward;
        }
    }
}

static void update_airborne(CharacterState *c, uint32_t input) {
    c->vy += GRAVITY;
    /* Air friction only on dash jumps — normal jumps preserve full momentum */
    if (c->dash_jump) {
        if (c->vx > 0) { c->vx -= AIR_FRICTION; if (c->vx < 0) c->vx = 0; }
        else if (c->vx < 0) { c->vx += AIR_FRICTION; if (c->vx > 0) c->vx = 0; }
    }
    c->x += c->vx;
    c->y += c->vy;

    int ground_top = GROUND_Y - c->height;
    if (c->y >= FIXED_FROM_INT(ground_top)) {
        /* Check for ground bounce */
        if (c->ground_bounce_pending) {
            /* Apply ground bounce: upward velocity, stay airborne */
            c->y = FIXED_FROM_INT(ground_top);
            c->vy = FIXED_FROM_INT(-10);  /* Bounce up */
            c->ground_bounce_pending = FALSE;
            c->ground_bounce_used = TRUE;
            c->on_ground = FALSE;
            /* Keep some horizontal momentum */
            c->vx = c->vx * 2 / 3;
            set_anim(c, ANIM_JUMP);
        } else {
            /* Normal landing */
            c->y = FIXED_FROM_INT(ground_top);
            c->vy = 0;
            c->on_ground = TRUE;
            c->dash_jump = FALSE;
            c->super_jump = FALSE;
            c->state = STATE_LANDING;
            c->state_frame = 0;
            set_anim(c, ANIM_IDLE);
        }
    }

    clamp_stage_bounds(c);
}

static void update_landing(CharacterState *c, uint32_t input, const CharacterDef *def) {
    c->vx = 0;
    c->vy = 0;
    c->state_frame++;
    if (c->state_frame >= def->landing_frames) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        set_anim(c, ANIM_IDLE);
    }
}

/* Unified dash update — dir = 1 for forward, -1 for backward */
static void update_dash(CharacterState *c, uint32_t input, const CharacterDef *def, int dir) {
    int max_frames = (dir == 1) ? def->dash_forward_frames : def->dash_backward_frames;
    c->dashing = TRUE;
    c->state_frame++;
    /* Set velocity: burst start, decelerate over time */
    if (c->state_frame == 1) {
        c->vx = dir * def->dash_speed * c->facing;
    } else {
        fixed_t speed = dir * c->vx * c->facing; /* absolute speed */
        speed -= def->dash_friction;
        if (speed < def->dash_min_speed) speed = def->dash_min_speed;
        c->vx = dir * speed * c->facing;
    }
    /* Jump cancel from dash — keep dashing flag so jump squat carries momentum */
    if (INPUT_HAS(input, INPUT_UP)) {
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
        set_anim(c, ANIM_JUMP);
        /* Keep c->dashing = TRUE so jump_squat preserves dash vx */
        return;
    }
    /* Crouch cancel after minimum frames */
    if (c->state_frame >= DASH_CROUCH_CANCEL_FRAMES && INPUT_HAS(input, INPUT_DOWN)) {
        c->state = STATE_CROUCH;
        c->state_frame = 0;
        c->dashing = FALSE;
        c->vx = 0;
        enter_crouch(c);
        set_anim(c, ANIM_CROUCH);
        return;
    }
    if (c->state_frame >= max_frames) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->dashing = FALSE;
        c->vx = 0;
        set_anim(c, ANIM_IDLE);
    }
    /* Apply velocity */
    c->x += c->vx;
    clamp_stage_bounds(c);
}

/* Apply gravity and movement for airborne attack states */
static void apply_air_physics(CharacterState *c) {
    c->vy += GRAVITY;
    /* Air friction only on dash jumps — consistent with update_airborne */
    if (c->dash_jump) {
        if (c->vx > 0) { c->vx -= AIR_FRICTION; if (c->vx < 0) c->vx = 0; }
        else if (c->vx < 0) { c->vx += AIR_FRICTION; if (c->vx > 0) c->vx = 0; }
    }
    c->x += c->vx;
    c->y += c->vy;
    int ground_top = GROUND_Y - c->height;
    if (c->y >= FIXED_FROM_INT(ground_top)) {
        c->y = FIXED_FROM_INT(ground_top);
        c->vy = 0;
        c->on_ground = TRUE;
        c->dash_jump = FALSE;
        c->super_jump = FALSE;
    }
    clamp_stage_bounds(c);
}

/* Check if air attack just landed.
 * - During STARTUP: cancel the attack (you mistimed it).
 * - During ACTIVE: keep the attack going — the hitbox persists through landing.
 *   This is how jump-in attacks work in every fighting game.
 * - During RECOVERY: cancel into landing state. */
static int air_attack_landed(PlayerState *p, CharacterState *c) {
    if (!p->attack_from_air || !c->on_ground) return 0;

    /* Active frames survive landing — don't cancel */
    if (c->state == STATE_ATTACK_ACTIVE) {
        c->dash_jump = FALSE;
        return 0;
    }

    /* Startup or recovery: cancel into landing */
    c->state = STATE_LANDING;
    c->state_frame = 0;
    c->vx = 0;
    p->current_attack = NULL;
    p->attack_from_air = FALSE;
    p->attack_from_crouch = FALSE;
    set_anim(c, ANIM_IDLE);
    return 1;
}

static void update_attack_startup(PlayerState *p, CharacterState *c, const struct MoveData *move) {
    if (!c->on_ground) apply_air_physics(c);
    if (air_attack_landed(p, c)) return;
    c->state_frame++;
    if (c->state_frame >= move->startup_frames) {
        c->state = STATE_ATTACK_ACTIVE;
        c->state_frame = 0;
    }
}

static void update_attack_active(PlayerState *p, CharacterState *c, const struct MoveData *move) {
    if (!c->on_ground) apply_air_physics(c);
    if (air_attack_landed(p, c)) return;
    c->state_frame++;
    int active_frames = move->active_end - move->active_start + 1;
    if (c->state_frame >= active_frames) {
        c->state = STATE_ATTACK_RECOVERY;
        c->state_frame = 0;
    }
}

static void update_attack_recovery(PlayerState *p, CharacterState *c, const struct MoveData *move) {
    if (!c->on_ground) apply_air_physics(c);
    /* Air attack that landed during active frames — now in recovery on ground.
     * Transition to landing immediately (attack is done, hitbox served its purpose). */
    if (p->attack_from_air && c->on_ground) {
        c->state = STATE_LANDING;
        c->state_frame = 0;
        c->vx = 0;
        p->current_attack = NULL;
        p->attack_from_air = FALSE;
        p->attack_from_crouch = FALSE;
        set_anim(c, ANIM_IDLE);
        return;
    }
    c->state_frame++;
    if (c->state_frame >= move->recovery_frames) {
        if (!c->on_ground) {
            c->state = STATE_AIRBORNE;
            set_anim(c, ANIM_JUMP);
        } else if (p->attack_from_crouch) {
            c->state = STATE_CROUCH;
            set_anim(c, ANIM_CROUCH);
            c->anim_frame = 999;  /* last frame — already crouched */
            /* Physically enter crouch if not already crouched
             * (5-series moves uncrouch during startup, so we must re-crouch) */
            if (!c->crouching) {
                enter_crouch(c);
            }
        } else {
            c->state = STATE_IDLE;
            set_anim(c, ANIM_IDLE);
        }
        c->state_frame = 0;
        p->attack_from_crouch = FALSE;
        p->attack_from_air = FALSE;
    }
}

#define HITSTUN_FRICTION FIXED_FROM_INT(1)

static void update_hitstun(CharacterState *c) {
    set_anim(c, ANIM_HITSTUN);

    if (!c->on_ground) {
        /* Airborne hitstun (launched): apply gravity, land on ground */
        c->vy += GRAVITY;
        c->x += c->vx;
        c->y += c->vy;

        /* Horizontal friction in air */
        if (c->vx > 0) { c->vx -= AIR_FRICTION; if (c->vx < 0) c->vx = 0; }
        else if (c->vx < 0) { c->vx += AIR_FRICTION; if (c->vx > 0) c->vx = 0; }

        int ground_top = GROUND_Y - c->height;
        if (c->y >= FIXED_FROM_INT(ground_top)) {
            c->y = FIXED_FROM_INT(ground_top);
            c->vy = 0;
            c->on_ground = TRUE;
            /* Land into knockdown (launched opponents don't just stand up) */
            c->state = STATE_KNOCKDOWN;
            c->state_frame = 0;
            c->in_hitstun = FALSE;
            c->vx = 0;
            return;
        }
    } else {
        /* Grounded hitstun: no vertical movement */
        c->vy = 0;

        /* Apply decelerating pushback — slides backward, slows to stop */
        c->x += c->vx;
        if (c->vx > 0) { c->vx -= HITSTUN_FRICTION; if (c->vx < 0) c->vx = 0; }
        else if (c->vx < 0) { c->vx += HITSTUN_FRICTION; if (c->vx > 0) c->vx = 0; }
    }

    clamp_stage_bounds(c);

    c->state_frame++;
    if (c->state_frame >= c->hitstun_remaining) {
        c->in_hitstun = FALSE;
        c->vx = 0;
        if (!c->on_ground) {
            /* Airborne hitstun expired: fall freely until landing */
            c->state = STATE_AIRBORNE;
            c->state_frame = 0;
            set_anim(c, ANIM_JUMP);
        } else if (c->crouching) {
            c->state = STATE_CROUCH;
            c->state_frame = 0;
            set_anim(c, ANIM_CROUCH);
            c->anim_frame = 999;  /* last frame — already crouched */
        } else {
            c->state = STATE_IDLE;
            c->state_frame = 0;
            set_anim(c, ANIM_IDLE);
        }
    }
}

static void update_blockstun(CharacterState *c) {
    /* Decay pushback velocity (friction) instead of zeroing it */
    c->vx = (c->vx * 85) / 100;  /* ~15% friction per frame */
    c->vy = 0;
    c->x += c->vx;
    set_anim(c, ANIM_BLOCKSTUN);
    c->state_frame++;
    if (c->state_frame >= c->blockstun_remaining) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->in_blockstun = FALSE;
        c->vx = 0;
        set_anim(c, ANIM_IDLE);
    }
}

#define KNOCKDOWN_FRAMES 30  /* total time on ground before wakeup */
#define WAKEUP_FRAMES 8      /* invincible wakeup animation */

static void update_knockdown(CharacterState *c) {
    set_anim(c, ANIM_KNOCKDOWN);

    /* Apply decelerating pushback */
    c->x += c->vx;
    if (c->vx > 0) { c->vx -= HITSTUN_FRICTION; if (c->vx < 0) c->vx = 0; }
    else if (c->vx < 0) { c->vx += HITSTUN_FRICTION; if (c->vx > 0) c->vx = 0; }
    clamp_stage_bounds(c);

    c->state_frame++;
    /* TODO: OTG window check here (combo_can_otg) */
    if (c->state_frame >= KNOCKDOWN_FRAMES) {
        c->state = STATE_IDLE;
        c->state_frame = 0;
        c->in_hitstun = FALSE;
        c->vx = 0;
        set_anim(c, ANIM_IDLE);
        /* TODO: wakeup invincibility frames */
    }
}

/* --- Cancel hierarchy helpers --- */

/* Can the given action category cancel at this level? */
static int can_cancel(CancelLevel level, ActionType action) {
    return level != CANCEL_NONE && (int)action >= (int)level;
}

/* Determine cancel level from current attack on hit.
 *
 * Engine rule: ALL normals on hit grant at least CANCEL_BY_SPECIAL (special + super).
 * MOVE_PROP_CHAIN additionally allows normal→normal chains.
 * Specials only get super cancel if MOVE_PROP_SUPER_CANCEL is set. */
static CancelLevel cancel_level_from_move(const PlayerState *p) {
    if (!p->hit_confirmed) return CANCEL_NONE;
    const struct MoveData *move = p->current_attack;
    if (!move) return CANCEL_NONE;

    if (move->move_type == MOVE_TYPE_NORMAL) {
        /* All normals: special cancel on hit is guaranteed by the engine.
         * CHAIN flag adds normal→normal chains on top. */
        return (move->properties & MOVE_PROP_CHAIN) ? CANCEL_BY_NORMAL : CANCEL_BY_SPECIAL;
    }

    /* Specials: only super cancel if flagged */
    if (move->properties & MOVE_PROP_SUPER_CANCEL) return CANCEL_BY_SUPER;
    return CANCEL_NONE;
}

/* Full cancel validation: category check + chain strength ordering.
 * Chain normals (MOVE_PROP_CHAIN) enforce ascending strength (L->M->H),
 * except lights (strength 0) which can self-chain (L->L->L).
 * Specials/supers bypass strength checks entirely. */
static int can_cancel_into(const PlayerState *p, CancelLevel lvl, const struct MoveData *target) {
    if (!target) return 0;
    ActionType action;
    switch (target->move_type) {
        case MOVE_TYPE_SUPER:   action = ACTION_SUPER; break;
        case MOVE_TYPE_SPECIAL: action = ACTION_SPECIAL; break;
        default:                action = ACTION_NORMAL; break;
    }
    /* S launcher (MOVE_PROP_LAUNCHER normal) is cancellable-into at special tier.
     * Any normal on hit grants CANCEL_BY_SPECIAL, so any normal → 5S works. */
    if (action == ACTION_NORMAL && (target->properties & MOVE_PROP_LAUNCHER))
        action = ACTION_SPECIAL;
    if (!can_cancel(lvl, action)) return 0;
    /* Chain ordering: normal→normal via MOVE_PROP_CHAIN.
     * Lights (strength 0) can self-chain: L→L→L (pushback is the natural limiter).
     * Mediums and heavies require strictly ascending strength. */
    if (action == ACTION_NORMAL && p->current_attack
        && (p->current_attack->properties & MOVE_PROP_CHAIN)) {
        if (target->strength < p->current_attack->strength)
            return 0;  /* Can't chain down (H→M, M→L) */
        if (target->strength == p->current_attack->strength
            && p->current_attack->strength > 0)
            return 0;  /* Same strength blocked for M/H (no M→M, H→H) */
    }
    return 1;
}

/* Get the cancel level for the player's current state */
static CancelLevel player_get_cancel_level(const PlayerState *p) {
    const CharacterState *c = &p->chars[p->active_char];
    switch (c->state) {
        case STATE_IDLE:
        case STATE_WALK_FORWARD:
        case STATE_WALK_BACKWARD:
        case STATE_CROUCH:
        case STATE_AIRBORNE:
            return CANCEL_FREE;
        case STATE_DASH_FORWARD:
        case STATE_DASH_BACKWARD:
            return (c->state_frame >= DASH_BUTTON_CANCEL_FRAMES)
                   ? CANCEL_BY_NORMAL : CANCEL_NONE;
        case STATE_ATTACK_STARTUP:
            return CANCEL_NONE;
        case STATE_ATTACK_ACTIVE:
        case STATE_ATTACK_RECOVERY:
            return cancel_level_from_move(p);
        default:
            return CANCEL_NONE;
    }
}

/* Check if the current character state satisfies a move's stance requirement.
 * stance==0 (STANCE_ANY) always passes. Otherwise all set bits must match. */
static int stance_ok(const CharacterState *c, int stance) {
    if (stance == STANCE_ANY) return 1;
    if ((stance & STANCE_GROUNDED) && !c->on_ground) return 0;
    if ((stance & STANCE_AIRBORNE) && c->on_ground) return 0;
    if ((stance & STANCE_STANDING) && c->crouching) return 0;
    return 1;
}

/* Start a new attack — replaces the 4 duplicated attack-start blocks */
static void start_attack_move(PlayerState *p, const struct MoveData *move) {
    CharacterState *c = &p->chars[p->active_char];
    p->current_attack = move;
    p->attack_hit_id++;
    p->opponent_hits[0] = -1;
    p->opponent_hits[1] = -1;
    p->hit_confirmed = 0;
    p->attack_from_crouch = c->crouching;
    p->attack_from_air = !c->on_ground;
    /* Uncrouch for standing attacks only */
    if (c->crouching && move->name && move->name[0] != '2') {
        uncrouch(c);
    }
    c->state = STATE_ATTACK_STARTUP;
    c->state_frame = 0;
    c->dashing = FALSE;
    if (!p->attack_from_air)
        c->vx = 0;
    /* Set attack animation — force reset even if same move (e.g. mashing jab) */
    if (move->anim_id >= 0) {
        c->current_anim = -1;  /* invalidate so set_anim always resets */
        set_anim(c, move->anim_id);
    }
}

void player_update(PlayerState *p, uint32_t input, const InputBuffer *input_buf) {
    CharacterState *c = &p->chars[p->active_char];
    p->frame_counter++;

    uint32_t pressed = input & ~p->prev_input;

    /* Record button press frames for two-button dash leniency */
    if (pressed & INPUT_LIGHT) p->button_press_frame[0] = p->frame_counter;
    if (pressed & INPUT_MEDIUM) p->button_press_frame[1] = p->frame_counter;
    if (pressed & INPUT_HEAVY) p->button_press_frame[2] = p->frame_counter;

    /* Track last frame DOWN was held (for 28 super jump) */
    if (INPUT_HAS(input, INPUT_DOWN)) p->last_down_frame = p->frame_counter;

    CancelLevel lvl = player_get_cancel_level(p);
    int action_taken = 0;

    /* --- Pending attack buffer (from idle/walk: delays attack commit to allow late dash) --- */
    if (p->pending_attack) {
        /* If a motion+button arrives during the pending window, don't treat it
         * as a two-button dash. Commit the pending normal immediately (using the
         * direction stored at press time) so the motion can go through the
         * cancel chain on the next frame via combo buffer. */
        int motion_override = 0;
        if ((pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY)) && input_buf) {
            MotionType motion = input_detect_motion(input_buf, c->facing);
            if (motion != MOTION_NONE) motion_override = 1;
        }

        if (!motion_override && two_button_dash_input(p, pressed)) {
            if (c->crouching) uncrouch(c);
            start_dash(c, input);
            p->pending_attack = 0;
            clear_dash_buttons(p);
        } else if (motion_override || p->frame_counter - p->pending_attack_frame >= TWO_BUTTON_DASH_LENIENCY) {
            /* Use stored direction from press time, not current live input */
            uint32_t resolve_input = p->pending_attack_dir
                | (input & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL));
            const struct MoveData *move = resolve_normal(p, p->pending_attack, resolve_input);
            if (move) {
                start_attack_move(p, move);
            }
            p->pending_attack = 0;
        }
        action_taken = 1; /* Skip cancel chain while pending */
    }

    /* --- Combo buffer: replay stored input when cancel level opens --- */
    if (!action_taken && p->buffered_button) {
        if (p->frame_counter - p->buffered_button_frame < COMBO_BUFFER_WINDOW) {
            {
                const struct MoveData *move = resolve_normal(p, p->buffered_button, input);
                if (move && can_cancel_into(p, lvl, move)) {
                    start_attack_move(p, move);
                    p->buffered_button = 0;
                    action_taken = 1;
                } else if (!can_cancel(lvl, ACTION_NORMAL)) {
                    /* Cancel level not open yet — keep buffer alive */
                } else {
                    p->buffered_button = 0; /* Level open but chain rejected */
                    action_taken = 1;
                }
            }
        } else {
            p->buffered_button = 0; /* Expired */
        }
    }

    /* --- Centralized cancel priority chain --- */
    if (!action_taken) {
    /* super > special > two-button dash > normal attack > movement */

    /* 1. Super: motion + L+M pressed together */
    if (!action_taken
        && (pressed & INPUT_THROWN) == INPUT_THROWN
        && can_cancel(lvl, ACTION_SUPER)
        && input_buf) {
        MotionType motion = input_detect_motion(input_buf, c->facing);
        if (motion != MOTION_NONE) {
            const MoveData *super = character_get_super(p->character_id, 1);
            if (super && p->meter >= super->meter_cost && stance_ok(c, super->stance)) {
                p->meter -= super->meter_cost;
                start_attack_move(p, super);
                action_taken = 1;
            }
        }
    }

    /* 2. Special move: motion + attack button */
    if (!action_taken
        && (pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY))
        && can_cancel(lvl, ACTION_SPECIAL)
        && input_buf) {
        MotionType motion = input_detect_motion(input_buf, c->facing);
        if (motion != MOTION_NONE) {
            const MoveData *special = character_get_special(p->character_id, motion,
                pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY));
            if (special && stance_ok(c, special->stance)) {
                start_attack_move(p, special);
                action_taken = 1;
            }
        }
    }

    /* 3. Two-button dash: two buttons pressed within leniency window */
    if (!action_taken && two_button_dash_input(p, pressed)
        && can_cancel(lvl, ACTION_NORMAL)) {
        if (c->crouching) uncrouch(c);
        start_dash(c, input);
        p->pending_attack = 0;
        clear_dash_buttons(p);
        action_taken = 1;
    }
    /* 4. Single normal attack (newly pressed) */
    if (!action_taken && (pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL))
             && can_cancel(lvl, ACTION_NORMAL)) {
        /* Pending buffer only exists to disambiguate single press vs two-button dash.
         * Only delay when two-button dash is actually possible (grounded neutral). */
        int can_two_button_dash = (lvl == CANCEL_FREE && c->on_ground);
        if (can_two_button_dash) {
            p->pending_attack = pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL);
            p->pending_attack_frame = p->frame_counter;
            p->pending_attack_dir = input & (INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT);
        } else if (lvl == CANCEL_FREE) {
            /* No dash possible (airborne, etc.) — commit immediately */
            uint32_t atk_btn = pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL);
            const struct MoveData *move = resolve_normal(p, atk_btn, input);
            if (move) {
                start_attack_move(p, move);
            }
        } else {
            /* From other cancelable states (on-hit): commit immediately */
            uint32_t atk_btn = pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL);
            const struct MoveData *move = resolve_normal(p, atk_btn, input);
            if (move && can_cancel_into(p, lvl, move)) {
                start_attack_move(p, move);
            }
        }
        action_taken = 1;
    }
    /* 5. Jump cancel: MOVE_PROP_JUMP_CANCEL + hit_confirmed + UP */
    if (!action_taken && INPUT_HAS(input, INPUT_UP)
             && p->hit_confirmed && p->current_attack
             && (p->current_attack->properties & MOVE_PROP_JUMP_CANCEL)
             && (c->state == STATE_ATTACK_ACTIVE || c->state == STATE_ATTACK_RECOVERY)
             && c->on_ground) {
        if (p->attack_from_crouch) {
            uncrouch(c);
        }
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
        c->super_jump = FALSE;
        set_anim(c, ANIM_JUMP);
        p->current_attack = NULL;
        p->hit_confirmed = 0;
        p->attack_from_crouch = FALSE;
        action_taken = 1;
    }
    /* 5b. Super jump cancel: MOVE_PROP_SUPER_JUMP_CANCEL + hit_confirmed + UP */
    if (!action_taken && INPUT_HAS(input, INPUT_UP)
             && p->hit_confirmed && p->current_attack
             && (p->current_attack->properties & MOVE_PROP_SUPER_JUMP_CANCEL)
             && (c->state == STATE_ATTACK_ACTIVE || c->state == STATE_ATTACK_RECOVERY)
             && c->on_ground) {
        if (p->attack_from_crouch) {
            uncrouch(c);
        }
        c->state = STATE_JUMP_SQUAT;
        c->state_frame = 0;
        c->super_jump = TRUE;
        set_anim(c, ANIM_JUMP);
        p->current_attack = NULL;
        p->hit_confirmed = 0;
        p->attack_from_crouch = FALSE;
        action_taken = 1;
    }
    /* 6. Movement: double-tap dash, wavedash */
    if (!action_taken && can_cancel(lvl, ACTION_MOVEMENT)) {
        if (c->state == STATE_CROUCH && !INPUT_HAS(input, INPUT_DOWN)) {
            check_dash_input(p, c, input);
            if (c->state == STATE_DASH_FORWARD || c->state == STATE_DASH_BACKWARD) {
                uncrouch(c);
                action_taken = 1;
            }
        } else if (c->state != STATE_CROUCH) {
            CharacterStateEnum prev_state = c->state;
            check_dash_input(p, c, input);
            if (c->state != prev_state) action_taken = 1;
        }
    }
    } /* end cancel priority chain */

    /* --- Store failed press in combo buffer --- */
    if (!action_taken && (pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL))) {
        p->buffered_button = pressed & (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY | INPUT_SPECIAL);
        p->buffered_button_frame = p->frame_counter;
    }

    /* --- State frame updates --- */
    const CharacterDef *def = get_char_def(p);
    CharacterStateEnum pre_state = c->state;
    switch (c->state) {
        case STATE_IDLE: update_idle(c, input, def); break;
        case STATE_WALK_FORWARD: update_walk(c, input, def, 1); break;
        case STATE_WALK_BACKWARD: update_walk(c, input, def, -1); break;
        case STATE_CROUCH: update_crouch(c, input); break;
        case STATE_JUMP_SQUAT: update_jump_squat(p, c, input, def); break;
        case STATE_AIRBORNE: update_airborne(c, input); break;
        case STATE_LANDING: update_landing(c, input, def); break;
        case STATE_DASH_FORWARD: update_dash(c, input, def, 1); break;
        case STATE_DASH_BACKWARD: update_dash(c, input, def, -1); break;
        case STATE_ATTACK_STARTUP:
            if (p->current_attack) update_attack_startup(p, c, p->current_attack);
            break;
        case STATE_ATTACK_ACTIVE:
            if (p->current_attack) update_attack_active(p, c, p->current_attack);
            break;
        case STATE_ATTACK_RECOVERY:
            if (p->current_attack) update_attack_recovery(p, c, p->current_attack);
            break;
        case STATE_HITSTUN: update_hitstun(c); break;
        case STATE_BLOCKSTUN: update_blockstun(c); break;
        case STATE_KNOCKDOWN: update_knockdown(c); break;
        default: break;
    }

    /* Super jump from neutral: down→up (28 input) triggers super jump.
     * In MvC3, pressing down then up does a super jump. Detected when
     * entering JUMP_SQUAT and DOWN was held within the last few frames. */
    #define SUPER_JUMP_DOWN_WINDOW 4
    if (c->state == STATE_JUMP_SQUAT && c->state_frame == 0
        && pre_state != STATE_JUMP_SQUAT
        && !c->super_jump
        && (p->frame_counter - p->last_down_frame) <= SUPER_JUMP_DOWN_WINDOW) {
        c->super_jump = TRUE;
    }

    p->prev_input = input;
}

/* Can this character auto-turn to face the opponent?
 * Grounded neutral states + landing — airborne and attack states lock facing.
 * LANDING auto-faces so that buffered attacks after crossups go the right way. */
static int can_auto_face(const CharacterState *c) {
    switch (c->state) {
        case STATE_IDLE:
        case STATE_WALK_FORWARD:
        case STATE_WALK_BACKWARD:
        case STATE_CROUCH:
        case STATE_LANDING:
            return 1;
        default:
            return 0;
    }
}

void player_update_facing(PlayerState *p1, PlayerState *p2) {
    CharacterState *c1 = &p1->chars[p1->active_char];
    CharacterState *c2 = &p2->chars[p2->active_char];

    fixed_t p1_center = c1->x + FIXED_FROM_INT(c1->width / 2);
    fixed_t p2_center = c2->x + FIXED_FROM_INT(c2->width / 2);
    int new_facing_1 = (p1_center < p2_center) ? 1 : -1;

    if (can_auto_face(c1)) c1->facing = new_facing_1;
    if (can_auto_face(c2)) c2->facing = -new_facing_1;
}

void player_resolve_collisions(PlayerState *p1, PlayerState *p2) {
    CharacterState *c1 = &p1->chars[p1->active_char];
    CharacterState *c2 = &p2->chars[p2->active_char];

    /* No pushbox while either player is airborne — allows crossups and jump-overs.
     * On landing, the normal ground pushbox resolves which side you end up on. */
    if (!c1->on_ground || !c2->on_ground) return;

    /* Ground-to-ground pushbox: simple AABB collision */
    int c1_left = FIXED_TO_INT(c1->x);
    int c1_right = c1_left + c1->width;
    int c2_left = FIXED_TO_INT(c2->x);
    int c2_right = c2_left + c2->width;

    /* Check for overlap */
    if (c1_left < c2_right && c1_right > c2_left) {
        /* Push them apart */
        int overlap = (c1_right - c2_left < c2_right - c1_left) ?
                      (c1_right - c2_left) : (c2_right - c1_left);
        int push = (overlap + 1) / 2;  /* ceiling division — just enough to separate */

        if (c1->x < c2->x) {
            c1->x -= FIXED_FROM_INT(push);
            c2->x += FIXED_FROM_INT(push);
        } else {
            c1->x += FIXED_FROM_INT(push);
            c2->x -= FIXED_FROM_INT(push);
        }

        /* Re-apply stage bounds */
        clamp_stage_bounds(c1);
        clamp_stage_bounds(c2);
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
        case STATE_ATTACK_STARTUP: return "ATK_START";
        case STATE_ATTACK_ACTIVE: return "ATK_ACTIVE";
        case STATE_ATTACK_RECOVERY: return "ATK_RECV";
        case STATE_HITSTUN: return "HITSTUN";
        case STATE_BLOCKSTUN: return "BLOCKSTUN";
        case STATE_BLOCK_STANDING: return "BLOCK_ST";
        case STATE_BLOCK_CROUCHING: return "BLOCK_CR";
        case STATE_KNOCKDOWN: return "KNOCKDOWN";
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
