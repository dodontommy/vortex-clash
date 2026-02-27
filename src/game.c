#include "game.h"
#include "player.h"
#include "projectile.h"
#include "sprite.h"
#include "render.h"
#include "hitbox.h"
#include "character.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

/* Snapback move data — universal, costs 1 bar */
static const MoveData SNAPBACK_MOVE = {
    .name = "Snapback",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 40,
    .startup_frames = 20,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 17,
    .damage = 800,
    .hitstun = 20,
    .blockstun = 12,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(6),
    .knockback_y = 0,
    .x_offset = 50,
    .y_offset = 15,
    .width = 45,
    .height = 40,
    .properties = MOVE_PROP_NONE,
    .stance = STANCE_GROUNDED,
    .strength = 2,
    .meter_cost = 1000,
    .meter_gain = 0,
    .anim_id = -1
};

/* Clamp character position to stage boundaries (mirrors player.c) */
static void clamp_stage_bounds_game(CharacterState *c) {
    if (c->x < 0) c->x = 0;
    if (c->x > FIXED_FROM_INT(STAGE_WIDTH - c->width))
        c->x = FIXED_FROM_INT(STAGE_WIDTH - c->width);
}

static void trigger_screen_shake(GameRenderState *render, int damage) {
    if (damage >= 2000) {
        render->shake_amplitude = 5.0f;
        render->shake_frames = 7;
    } else if (damage >= 1500) {
        render->shake_amplitude = 3.0f;
        render->shake_frames = 5;
    } else {
        render->shake_amplitude = 1.5f;
        render->shake_frames = 3;
    }
    render->shake_frames_max = render->shake_frames;
}

/* Result of a hit collision check (no state mutation) */
typedef struct {
    int hit;            /* 1 = collision detected */
    int blocking;       /* 1 = defender is blocking */
    int is_projectile;  /* 1 = projectile spawn instead of melee */
    int otg_hit;        /* 1 = hit a knocked-down opponent */
} HitCheckResult;

/* Phase 1: Check collision without mutating state. Returns result. */
static HitCheckResult check_attack_collision(GameState *game, PlayerState *attacker, PlayerState *defender, uint32_t defender_input) {
    HitCheckResult result = {0};
    CharacterState *a = &attacker->chars[attacker->active_char];
    CharacterState *d = &defender->chars[defender->active_char];

    if (a->state != STATE_ATTACK_ACTIVE || !attacker->current_attack)
        return result;

    int opp_idx = defender->player_id - 1;
    if (attacker->opponent_hits[opp_idx] == attacker->attack_hit_id)
        return result;

    const struct MoveData *move = attacker->current_attack;

    if (d->wakeup_timer > 0)
        return result;

    if (d->state == STATE_KNOCKDOWN) {
        if (d->state_frame < 5 || d->state_frame > 20)
            return result;
        if (!(move->properties & MOVE_PROP_OTG))
            return result;
        if (!combo_can_otg(&attacker->combo))
            return result;
        result.otg_hit = 1;
    }

    /* Projectile moves: flag for spawn (applied in phase 2) */
    if (move->properties & MOVE_PROP_PROJECTILE) {
        result.hit = 1;
        result.is_projectile = 1;
        return result;
    }

    Hitbox hb;
    hb.x = move->x_offset;
    hb.y = move->y_offset;
    hb.width = move->width;
    hb.height = move->height;
    hb.damage = move->damage;
    hb.hit_id = attacker->attack_hit_id;

    Hurtbox hurt;
    hurtbox_create(d, &hurt);

    if (hitbox_check_collision(&hb, &hurt, FIXED_TO_INT(a->x), FIXED_TO_INT(a->y), a->facing)) {
        result.hit = 1;
        result.blocking = is_blocking(d, a, defender_input, move->hit_type);
    }
    return result;
}

/* Phase 2: Apply hit result (mutates state). */
static void apply_attack_hit(GameState *game, GameRenderState *render, PlayerState *attacker, PlayerState *defender, HitCheckResult *result) {
    if (!result->hit) return;

    CharacterState *a = &attacker->chars[attacker->active_char];
    CharacterState *d = &defender->chars[defender->active_char];
    const struct MoveData *move = attacker->current_attack;
    int opp_idx = defender->player_id - 1;

    if (result->is_projectile) {
        int owner = attacker->player_id - 1;
        projectile_spawn(&game->projectiles, owner, attacker, move, d);
        attacker->opponent_hits[opp_idx] = attacker->attack_hit_id;
        return;
    }

    int saved_hits = attacker->combo.hit_count;
    hitbox_resolve_hit(a, d, move, result->blocking, &attacker->combo, &defender->combo, &game->hitstop_frames,
                       game->training.active && game->training.counter_hit);

    if (result->blocking && game->training.active) {
        attacker->combo.hit_count = saved_hits;
    }

    if (!result->blocking) {
        attacker->hit_confirmed = 1;
        trigger_screen_shake(render, move->damage);
        /* Reset damage drain delay for the defender's active character */
        render->damage_drain_delay[defender->player_id - 1][defender->active_char] = 30;

        /* Snapback on hit: force opponent character swap */
        if (move == &SNAPBACK_MOVE) {
            int partner_idx = 1 - defender->active_char;
            CharacterState *opp_partner = &defender->chars[partner_idx];
            if (opp_partner->hp > 0) {
                /* Force swap: partner flies in from behind */
                CharacterState *old_point = &defender->chars[defender->active_char];
                int snap_facing = old_point->facing;
                fixed_t spawn_x = old_point->x - FIXED_FROM_INT(200 * snap_facing);
                defender->active_char = partner_idx;
                opp_partner->state = STATE_INCOMING_FALL;
                opp_partner->state_frame = 0;
                opp_partner->facing = snap_facing;
                opp_partner->x = spawn_x;
                opp_partner->y = FIXED_FROM_INT(GROUND_Y - opp_partner->standing_height - 120);
                opp_partner->on_ground = FALSE;
                opp_partner->vy = 0;
                opp_partner->height = opp_partner->standing_height;
                opp_partner->crouching = FALSE;
                opp_partner->vx = FIXED_FROM_INT(10) * opp_partner->facing;
                defender->current_attack = NULL;
                defender->hit_confirmed = 0;
            }
        }
    }

    attacker->meter += move->meter_gain;
    if (attacker->meter > MAX_METER) attacker->meter = MAX_METER;
    defender->meter += move->meter_gain / 2;
    if (defender->meter > MAX_METER) defender->meter = MAX_METER;

    if (result->otg_hit && !result->blocking) {
        combo_use_otg(&attacker->combo);
    }

    attacker->opponent_hits[opp_idx] = attacker->attack_hit_id;
}

/* Sync sprite anim params + advance animation for a player */
static void update_player_anim(SpriteSet *spr, PlayerState *p) {
    CharacterState *c = &p->chars[p->active_char];
    sprite_sync_anim(spr, c->current_anim,
                     &c->anim_frame, &c->anim_timer,
                     &c->anim_frame_duration, &c->anim_frame_count,
                     &c->anim_looping);
    advance_anim(c);
}

static void camera_init(GameRenderState *render) {
    render->camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT * 0.65f };
    render->camera.rotation = 0.0f;
    render->camera.zoom = 2.0f;
    render->camera.target = (Vector2){ STAGE_WIDTH / 2.0f, GROUND_Y - 80.0f };
}

static void camera_update(GameRenderState *render, const GameState *game) {
    const CharacterState *c1 = &game->players[0].chars[game->players[0].active_char];
    const CharacterState *c2 = &game->players[1].chars[game->players[1].active_char];

    float p1_cx = (float)FIXED_TO_INT(c1->x) + c1->width / 2.0f;
    float p2_cx = (float)FIXED_TO_INT(c2->x) + c2->width / 2.0f;
    float mid_x = (p1_cx + p2_cx) / 2.0f;

    float p1_y = (float)FIXED_TO_INT(c1->y);
    float p2_y = (float)FIXED_TO_INT(c2->y);
    float min_y = p1_y < p2_y ? p1_y : p2_y;
    float target_y = (GROUND_Y - 80.0f + min_y) / 2.0f;
    if (target_y > GROUND_Y - 80.0f) target_y = GROUND_Y - 80.0f;

    float dist = p1_cx > p2_cx ? p1_cx - p2_cx : p2_cx - p1_cx;
    float needed_width = dist + CAMERA_PADDING;
    float desired_zoom = SCREEN_WIDTH / needed_width;
    if (desired_zoom < CAMERA_ZOOM_MIN) desired_zoom = CAMERA_ZOOM_MIN;
    if (desired_zoom > CAMERA_ZOOM_MAX) desired_zoom = CAMERA_ZOOM_MAX;

    float lerp = 0.1f;
    render->camera.target.x += (mid_x - render->camera.target.x) * lerp;
    render->camera.target.y += (target_y - render->camera.target.y) * lerp;
    render->camera.zoom += (desired_zoom - render->camera.zoom) * lerp;

    float half_view_w = (SCREEN_WIDTH / render->camera.zoom) / 2.0f;
    if (render->camera.target.x - half_view_w < 0)
        render->camera.target.x = half_view_w;
    if (render->camera.target.x + half_view_w > STAGE_WIDTH)
        render->camera.target.x = STAGE_WIDTH - half_view_w;
}

static void check_projectile_hits(GameState *game, GameRenderState *render, uint32_t def_inputs[2]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *proj = &game->projectiles.projectiles[i];
        if (!proj->active) continue;

        int def_idx = 1 - proj->owner;
        PlayerState *defender = &game->players[def_idx];
        PlayerState *attacker_p = &game->players[proj->owner];
        CharacterState *d = &defender->chars[defender->active_char];

        if (d->wakeup_timer > 0) continue;

        if (d->state == STATE_KNOCKDOWN) {
            if (d->state_frame < 5 || d->state_frame > 20)
                continue;
            if (!(proj->properties & MOVE_PROP_OTG))
                continue;
            if (!combo_can_otg(&attacker_p->combo))
                continue;
        }

        int px = FIXED_TO_INT(proj->x);
        int py = FIXED_TO_INT(proj->y);
        int dx = FIXED_TO_INT(d->x);
        int dy = FIXED_TO_INT(d->y);

        if (px < dx + d->width && px + proj->width > dx &&
            py < dy + d->height && py + proj->height > dy) {

            if (d->state == STATE_KNOCKDOWN) {
                combo_use_otg(&attacker_p->combo);
            }

            MoveData proj_move = {0};
            proj_move.damage = proj->damage;
            proj_move.hitstun = proj->hitstun;
            proj_move.blockstun = proj->blockstun;
            proj_move.chip_damage = proj->chip_damage;
            proj_move.knockback_x = proj->knockback_x;
            proj_move.knockback_y = proj->knockback_y;
            proj_move.properties = proj->properties;

            CharacterState fake_attacker = {0};
            fake_attacker.x = proj->x;
            fake_attacker.y = proj->y;

            int blocking = is_blocking(d, &fake_attacker, def_inputs[def_idx], proj->hit_type);

            hitbox_resolve_hit(&fake_attacker, d, &proj_move, blocking,
                               &attacker_p->combo, &defender->combo,
                               &game->hitstop_frames,
                               game->training.active && game->training.counter_hit);

            if (!blocking) {
                attacker_p->hit_confirmed = 1;
                trigger_screen_shake(render, proj->damage);
                render->damage_drain_delay[def_idx][defender->active_char] = 30;
            }

            attacker_p->meter += proj_move.meter_gain;
            if (attacker_p->meter > MAX_METER) attacker_p->meter = MAX_METER;
            defender->meter += proj_move.meter_gain / 2;
            if (defender->meter > MAX_METER) defender->meter = MAX_METER;

            proj->active = FALSE;
        }
    }
}

static void update_combo_display(GameRenderState *render, const GameState *game) {
    for (int i = 0; i < 2; i++) {
        if (game->players[i].combo.hit_count >= 2) {
            render->combo_display[i].display_hit_count = game->players[i].combo.hit_count;
            render->combo_display[i].display_damage = game->players[i].combo.total_damage;
            render->combo_display[i].fade_timer = 60;
        } else if (render->combo_display[i].fade_timer > 0) {
            render->combo_display[i].fade_timer--;
        }
    }
}

void game_init(GameState *game, GameRenderState *render) {
    player_init(&game->players[0], 1, STAGE_WIDTH / 2 - 200, GROUND_Y - 80, CHAR_RYKER);
    player_init(&game->players[1], 2, STAGE_WIDTH / 2 + 150, GROUND_Y - 80, CHAR_RYKER);
    input_init(&game->inputs[0]);
    input_init(&game->inputs[1]);
    projectile_init(&game->projectiles);
    game->hitstop_frames = 0;
    game->frame_count = 0;
    game->ko_winner = -1;
    game->ko_timer = 0;
    game->timer_frames = TIMER_START_FRAMES;
    memset(&game->training, 0, sizeof(TrainingState));

    /* Render state */
    sprite_load(&render->sprites[0], "ryker");
    sprite_load(&render->sprites[1], "ryker");
    render->running = TRUE;
    render->debug_draw = FALSE;
    memset(render->combo_display, 0, sizeof(render->combo_display));
    /* Init damage drain: start at full HP */
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            render->damage_display_hp[i][j] = (float)game->players[i].chars[j].max_hp;
            render->damage_drain_delay[i][j] = 0;
        }
    }
    input_bindings_init(&render->bindings[0], 1);
    input_bindings_init(&render->bindings[1], 2);
    camera_init(render);
    render_init();
    hitbox_init();
}

void game_update(GameState *game, GameRenderState *render) {
    /* KO freeze: countdown then reset */
    if (game->ko_timer > 0) {
        game->ko_timer--;
        if (game->ko_timer == 0) {
            player_init(&game->players[0], 1, STAGE_WIDTH / 2 - 200, GROUND_Y - 80, CHAR_RYKER);
            player_init(&game->players[1], 2, STAGE_WIDTH / 2 + 150, GROUND_Y - 80, CHAR_RYKER);
            game->ko_winner = -1;
            game->timer_frames = TIMER_START_FRAMES;
            /* Reset damage drain display */
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    render->damage_display_hp[i][j] = (float)game->players[i].chars[j].max_hp;
                    render->damage_drain_delay[i][j] = 0;
                }
            }
        }
        return;
    }

    /* Toggle debug draw */
    if (IsKeyPressed(KEY_F1)) render->debug_draw = !render->debug_draw;

    /* Toggle training mode / menu (F2 or gamepad Start/Menu button) */
    int menu_toggle = IsKeyPressed(KEY_F2) ||
                      (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT));
    if (menu_toggle) {
        if (!game->training.active) {
            game->training.active = TRUE;
            game->training.menu_open = TRUE;
        } else {
            game->training.menu_open = !game->training.menu_open;
        }
    }

    /* Training menu navigation (pauses game) */
    if (game->training.menu_open) {
        /* Remap sub-menu: listening for key/button press */
        if (game->training.remap_open && game->training.remap_listening) {
            int new_key = input_scan_key();
            if (new_key >= 0) {
                render->bindings[game->training.remap_target].buttons[game->training.remap_cursor].keyboard_key = new_key;
                game->training.remap_listening = FALSE;
            }
            int gp = game->training.remap_target;
            int new_btn = input_scan_gamepad(gp);
            if (new_btn >= 0) {
                render->bindings[game->training.remap_target].buttons[game->training.remap_cursor].gamepad_button = new_btn;
                game->training.remap_listening = FALSE;
            }
            /* Escape/B cancels listening */
            if (IsKeyPressed(KEY_ESCAPE) ||
                (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT))) {
                game->training.remap_listening = FALSE;
            }
            update_combo_display(render, game);
            return;
        }

        int nav_up = IsKeyPressed(KEY_UP) ||
                     (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP));
        int nav_down = IsKeyPressed(KEY_DOWN) ||
                       (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN));
        int nav_left = IsKeyPressed(KEY_LEFT) ||
                       (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT));
        int nav_right = IsKeyPressed(KEY_RIGHT) ||
                        (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
        int nav_confirm = IsKeyPressed(KEY_ENTER) ||
                          (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
        int nav_back = IsKeyPressed(KEY_ESCAPE) ||
                       (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));

        /* Remap sub-menu navigation */
        if (game->training.remap_open) {
            if (nav_up)   game->training.remap_cursor = (game->training.remap_cursor + REMAP_BUTTON_COUNT - 1) % REMAP_BUTTON_COUNT;
            if (nav_down) game->training.remap_cursor = (game->training.remap_cursor + 1) % REMAP_BUTTON_COUNT;
            if (nav_confirm) {
                game->training.remap_listening = TRUE;
            }
            if (nav_back) {
                game->training.remap_open = FALSE;
            }
            update_combo_display(render, game);
            return;
        }

        /* Main training menu: 6 items */
        if (nav_up)   game->training.cursor = (game->training.cursor + 5) % 6;
        if (nav_down)  game->training.cursor = (game->training.cursor + 1) % 6;
        if (nav_left || nav_right) {
            int dir = nav_right ? 1 : -1;
            switch (game->training.cursor) {
                case 0: game->training.block_mode = (DummyBlockMode)((game->training.block_mode + dir + BLOCK_MODE_COUNT) % BLOCK_MODE_COUNT); break;
                case 1: game->training.dummy_state = (DummyStateMode)((game->training.dummy_state + dir + DUMMY_STATE_COUNT) % DUMMY_STATE_COUNT); break;
                case 2: game->training.counter_hit = !game->training.counter_hit; break;
                case 3: game->training.hp_meter_reset = !game->training.hp_meter_reset; break;
            }
        }
        /* Confirm actions */
        if (nav_confirm) {
            if (game->training.cursor == 4) {
                game->training.remap_open = TRUE;
                game->training.remap_cursor = 0;
                game->training.remap_listening = FALSE;
                game->training.remap_target = 0;
            } else if (game->training.cursor == 5) {
                render->running = FALSE;
                return;
            } else {
                game->training.menu_open = FALSE;
            }
        }
        update_combo_display(render, game);
        return; /* Game paused while menu is open */
    }

    /* Tick hit flash (runs even during hitstop) */
    for (int i = 0; i < 2; i++) {
        CharacterState *cs = &game->players[i].chars[game->players[i].active_char];
        if (cs->hit_flash > 0) cs->hit_flash--;
    }

    /* Update hitstop */
    hitbox_update_hitstop(&game->hitstop_frames);
    if (hitbox_in_hitstop(&game->hitstop_frames))
        return;  /* Skip update during hitstop */

    /* Poll inputs — P1 uses keyboard + gamepad 0 */
    uint32_t p1_raw = input_poll_bound(1, INPUT_SOURCE_KEYBOARD, &render->bindings[0])
                     | input_poll_bound(1, INPUT_SOURCE_GAMEPAD, &render->bindings[0]);
    uint32_t p2_raw = input_poll_bound(2, INPUT_SOURCE_GAMEPAD, &render->bindings[1]);

    /* Training mode: override P2 (dummy) input */
    if (game->training.active) {
        CharacterState *p2c = &game->players[1].chars[game->players[1].active_char];
        switch (game->training.dummy_state) {
            case DUMMY_STAND:  p2_raw = 0; break;
            case DUMMY_CROUCH: p2_raw = INPUT_DOWN; break;
            case DUMMY_JUMP:   p2_raw = p2c->on_ground ? INPUT_UP : 0; break;
            default: p2_raw = 0; break;
        }
    }

    /* Update input buffers */
    input_update(&game->inputs[0], p1_raw);
    input_update(&game->inputs[1], p2_raw);

    /* Update facing FIRST — so inputs and attacks are processed with correct facing */
    player_update_facing(&game->players[0], &game->players[1]);

    /* --- DHC: during super active, A1 + motion + enough meter → swap into partner's super --- */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        CharacterState *c = &pl->chars[pl->active_char];
        if (c->state == STATE_ATTACK_ACTIVE && pl->current_attack &&
            pl->current_attack->move_type == MOVE_TYPE_SUPER) {
            if (input_button_pressed(&game->inputs[i], INPUT_A1)) {
                MotionType motion = input_detect_motion(&game->inputs[i], c->facing);
                if (motion != MOTION_NONE) {
                    const MoveData *partner_super = character_get_super(pl->character_id, 1);
                    int dhc_cost = (partner_super ? partner_super->meter_cost : 1000) + 1000;
                    int partner_idx = 1 - pl->active_char;
                    CharacterState *partner = &pl->chars[partner_idx];
                    if (partner->hp > 0 && pl->meter >= dhc_cost && partner_super) {
                        pl->meter -= dhc_cost;
                        /* Position partner at current point's location */
                        partner->x = c->x;
                        partner->y = c->y;
                        partner->facing = c->facing;
                        partner->on_ground = c->on_ground;
                        partner->height = c->height;
                        partner->crouching = FALSE;
                        /* Swap active */
                        pl->active_char = partner_idx;
                        /* Start partner's super */
                        pl->current_attack = partner_super;
                        pl->attack_hit_id++;
                        pl->opponent_hits[0] = -1;
                        pl->opponent_hits[1] = -1;
                        pl->hit_confirmed = 0;
                        pl->attack_from_air = !partner->on_ground;
                        pl->attack_from_crouch = FALSE;
                        partner->state = STATE_ATTACK_STARTUP;
                        partner->state_frame = 0;
                        partner->vx = 0;
                        /* Old character exits */
                        c->state = STATE_IDLE;
                        c->vx = 0;
                        c->vy = 0;
                    }
                }
            }
        }
    }

    /* --- Snapback: QCF+A1, 1 bar, forces opponent to swap characters --- */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        CharacterState *c = &pl->chars[pl->active_char];
        if (input_button_pressed(&game->inputs[i], INPUT_A1) && pl->meter >= 1000 &&
            c->on_ground && c->state == STATE_IDLE) {
            MotionType motion = input_detect_motion(&game->inputs[i], c->facing);
            if (motion == MOTION_QCF) {
                pl->meter -= 1000;
                pl->current_attack = &SNAPBACK_MOVE;
                pl->attack_hit_id++;
                pl->opponent_hits[0] = -1;
                pl->opponent_hits[1] = -1;
                pl->hit_confirmed = 0;
                pl->attack_from_crouch = FALSE;
                pl->attack_from_air = FALSE;
                c->state = STATE_ATTACK_STARTUP;
                c->state_frame = 0;
                c->vx = 0;
            }
        }
    }

    fixed_t p2x = game->players[1].chars[game->players[1].active_char].x;
    fixed_t p1x = game->players[0].chars[game->players[0].active_char].x;
    player_update(&game->players[0], input_get_current(&game->inputs[0]), &game->inputs[0], p2x);
    player_update(&game->players[1], input_get_current(&game->inputs[1]), &game->inputs[1], p1x);
    update_player_anim(&render->sprites[0], &game->players[0]);
    update_player_anim(&render->sprites[1], &game->players[1]);
    player_resolve_collisions(&game->players[0], &game->players[1]);

    /* Reset attacker combo when defender leaves hitstun (returns to neutral) */
    for (int i = 0; i < 2; i++) {
        int other = 1 - i;
        CharacterState *def_cs = &game->players[other].chars[game->players[other].active_char];
        if (!def_cs->in_hitstun && game->players[i].combo.hit_count > 0 &&
            def_cs->state != STATE_HITSTUN && def_cs->state != STATE_BLOCKSTUN) {
            combo_reset(&game->players[i].combo);
            if (game->training.active && game->training.hp_meter_reset) {
                for (int p = 0; p < 2; p++) {
                    CharacterState *ch = &game->players[p].chars[game->players[p].active_char];
                    ch->hp = ch->max_hp;
                }
            }
        }
    }

    /* Training mode: compute block override for P2 */
    uint32_t p2_def_input = p2_raw;
    if (game->training.active && game->training.block_mode != BLOCK_NONE) {
        CharacterState *p1c = &game->players[0].chars[game->players[0].active_char];
        CharacterState *p2c = &game->players[1].chars[game->players[1].active_char];
        uint32_t hold_back = (p1c->x < p2c->x) ? INPUT_RIGHT : INPUT_LEFT;
        switch (game->training.block_mode) {
            case BLOCK_ALL:
            case BLOCK_STAND:
                p2_def_input = hold_back;
                break;
            case BLOCK_CROUCH:
                p2_def_input = hold_back | INPUT_DOWN;
                break;
            case BLOCK_AFTER_FIRST:
                if (game->players[0].combo.hit_count >= 1)
                    p2_def_input = hold_back;
                break;
            default: break;
        }
    }

    /* Check for attack hits — snapshot both, then apply both (no P1 priority) */
    HitCheckResult hit_p1 = check_attack_collision(game, &game->players[0], &game->players[1], p2_def_input);
    HitCheckResult hit_p2 = check_attack_collision(game, &game->players[1], &game->players[0], p1_raw);
    apply_attack_hit(game, render, &game->players[0], &game->players[1], &hit_p1);
    apply_attack_hit(game, render, &game->players[1], &game->players[0], &hit_p2);

    /* Update projectiles: movement + despawn, then check hits */
    projectile_update(&game->projectiles);
    uint32_t proj_def_inputs[2] = { p1_raw, p2_def_input };
    check_projectile_hits(game, render, proj_def_inputs);

    /* KO check: team KO (both chars dead) or point KO (trigger incoming) */
    if (game->ko_winner < 0 && !game->training.active) {
        for (int i = 0; i < 2; i++) {
            PlayerState *pl = &game->players[i];
            CharacterState *point = &pl->chars[pl->active_char];
            if (point->hp <= 0) {
                int partner_idx = 1 - pl->active_char;
                CharacterState *partner = &pl->chars[partner_idx];
                if (partner->hp <= 0) {
                    /* Both dead — team eliminated */
                    game->ko_winner = 1 - i;
                    game->ko_timer = KO_FREEZE_FRAMES;
                } else {
                    /* Point KO, partner alive — fly in from behind */
                    point->state = STATE_KNOCKDOWN;
                    point->state_frame = 0;
                    int old_facing = point->facing;
                    fixed_t spawn_x = point->x - FIXED_FROM_INT(200 * old_facing);
                    pl->active_char = partner_idx;
                    partner->state = STATE_INCOMING_FALL;
                    partner->state_frame = 0;
                    partner->facing = old_facing;
                    partner->x = spawn_x;
                    partner->y = FIXED_FROM_INT(GROUND_Y - partner->standing_height - 120);
                    partner->on_ground = FALSE;
                    partner->vy = 0;
                    partner->height = partner->standing_height;
                    partner->crouching = FALSE;
                    partner->vx = FIXED_FROM_INT(10) * partner->facing;
                    pl->current_attack = NULL;
                    pl->hit_confirmed = 0;
                }
            }
        }
    }

    /* --- Match timer countdown --- */
    if (game->ko_winner < 0 && !game->training.active && game->timer_frames > 0) {
        game->timer_frames--;
        if (game->timer_frames == 0) {
            /* Time up: winner by total remaining HP */
            int p1_total = game->players[0].chars[0].hp + game->players[0].chars[1].hp;
            int p2_total = game->players[1].chars[0].hp + game->players[1].chars[1].hp;
            if (p1_total > p2_total) {
                game->ko_winner = 0;
            } else if (p2_total > p1_total) {
                game->ko_winner = 1;
            } else {
                game->ko_winner = 0; /* tie goes to P1 */
            }
            game->ko_timer = KO_FREEZE_FRAMES;
        }
    }

    /* --- Tag team: blue health recovery for off-screen characters --- */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        int off = 1 - pl->active_char;
        CharacterState *partner = &pl->chars[off];
        /* Recover only when off-screen and not incoming */
        if (!pl->assist_on_screen && partner->state != STATE_INCOMING_FALL) {
            if (partner->blue_hp > 0 && partner->hp < partner->max_hp) {
                int recover = 5;
                if (recover > partner->blue_hp) recover = partner->blue_hp;
                if (partner->hp + recover > partner->max_hp)
                    recover = partner->max_hp - partner->hp;
                partner->hp += recover;
                partner->blue_hp -= recover;
            }
        }
    }

    /* --- Tag team: tick assist cooldown --- */
    for (int i = 0; i < 2; i++) {
        if (game->players[i].assist_cooldown > 0)
            game->players[i].assist_cooldown--;
    }

    /* --- Tag team: A1 input detection --- */
    /* Hold A1: tag fires instantly at 15f. Release before 15f: assist. */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        CharacterState *c = &pl->chars[pl->active_char];
        uint32_t raw = input_get_current(&game->inputs[i]);

        if (INPUT_HAS(raw, INPUT_A1)) {
            pl->tag_hold_timer++;
            /* Hold reaches 15f: initiate tag immediately */
            if (pl->tag_hold_timer == 15) {
                int partner_idx = 1 - pl->active_char;
                CharacterState *partner = &pl->chars[partner_idx];
                int can_tag = (c->state == STATE_IDLE || c->state == STATE_WALK_FORWARD ||
                               c->state == STATE_WALK_BACKWARD || c->state == STATE_CROUCH) &&
                              c->on_ground &&
                              pl->assist_cooldown == 0 &&
                              !pl->assist_on_screen &&
                              partner->hp > 0;
                if (can_tag) {
                    c->state = STATE_TAG_DEPARTING;
                    c->state_frame = 0;
                    c->vx = 0;
                    if (c->crouching) {
                        c->y -= FIXED_FROM_INT(c->standing_height - c->crouch_height);
                        c->height = c->standing_height;
                        c->crouching = FALSE;
                    }
                }
            }
        } else if (pl->tag_hold_timer > 0) {
            /* Released before 15f: call assist */
            if (pl->tag_hold_timer < 15) {
                int partner_idx = 1 - pl->active_char;
                CharacterState *partner = &pl->chars[partner_idx];
                int can_assist = pl->assist_cooldown == 0 &&
                                 !pl->assist_on_screen &&
                                 partner->hp > 0;
                if (can_assist) {
                    const MoveData *assist_move = character_get_assist_move((CharacterId)pl->character_id);
                    if (assist_move) {
                        pl->assist_on_screen = 1;
                        pl->assist_hit = 0;
                        pl->assist_attack = assist_move;
                        pl->assist_hit_id++;
                        pl->assist_opponent_hits[0] = -1;
                        pl->assist_opponent_hits[1] = -1;
                        partner->state = STATE_ASSIST_ENTERING;
                        partner->state_frame = 0;
                        partner->on_ground = TRUE;
                        partner->y = FIXED_FROM_INT(GROUND_Y - partner->standing_height);
                        partner->height = partner->standing_height;
                        partner->crouching = FALSE;
                        partner->facing = c->facing;
                        partner->x = c->x - FIXED_FROM_INT(80 * c->facing);
                        partner->vx = FIXED_FROM_INT(6) * c->facing;
                    }
                }
            }
            pl->tag_hold_timer = 0;
        }
    }

    /* --- Tag team: update tag departing/incoming states --- */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        CharacterState *c = &pl->chars[pl->active_char];

        if (c->state == STATE_TAG_DEPARTING) {
            c->state_frame++;
            /* Departing char hops backward briefly */
            c->vx = FIXED_FROM_INT(-6) * c->facing;
            c->x += c->vx;
            if (c->state_frame >= 8) {
                int saved_facing = c->facing;
                fixed_t spawn_x = c->x - FIXED_FROM_INT(200 * saved_facing);
                /* Swap active_char */
                int new_active = 1 - pl->active_char;
                CharacterState *incoming = &pl->chars[new_active];
                pl->active_char = new_active;
                /* Fly in from behind, ~120px above ground */
                incoming->state = STATE_INCOMING_FALL;
                incoming->state_frame = 0;
                incoming->facing = saved_facing;
                incoming->x = spawn_x;
                incoming->y = FIXED_FROM_INT(GROUND_Y - incoming->standing_height - 120);
                incoming->on_ground = FALSE;
                incoming->height = incoming->standing_height;
                incoming->crouching = FALSE;
                incoming->vy = 0;
                incoming->vx = FIXED_FROM_INT(10) * incoming->facing;
                incoming->wakeup_timer = 5;
                /* Reset departure character state */
                c->state = STATE_IDLE;
                c->vx = 0;
                c->vy = 0;
                pl->current_attack = NULL;
                pl->hit_confirmed = 0;
            }
        }
    }

    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        CharacterState *c = &pl->chars[pl->active_char];

        if (c->state == STATE_INCOMING_FALL) {
            c->state_frame++;
            /* Fly in from the side, arc downward to ground */
            c->x += c->vx;
            c->vy += GRAVITY;
            c->y += c->vy;
            /* Decelerate horizontal speed */
            if (c->vx > 0) {
                c->vx -= FIXED_FROM_INT(1);
                if (c->vx < 0) c->vx = 0;
            } else if (c->vx < 0) {
                c->vx += FIXED_FROM_INT(1);
                if (c->vx > 0) c->vx = 0;
            }
            clamp_stage_bounds_game(c);
            /* Land on ground */
            int ground_top = GROUND_Y - c->height;
            if (c->y >= FIXED_FROM_INT(ground_top)) {
                c->y = FIXED_FROM_INT(ground_top);
                c->vy = 0;
                c->vx = 0;
                c->on_ground = TRUE;
                c->state = STATE_IDLE;
                c->state_frame = 0;
                c->wakeup_timer = 5;
            }
        }
    }

    /* --- Tag team: update assist state machine --- */
    for (int i = 0; i < 2; i++) {
        PlayerState *pl = &game->players[i];
        if (!pl->assist_on_screen) continue;
        int partner_idx = 1 - pl->active_char;
        CharacterState *assist = &pl->chars[partner_idx];
        CharacterState *point = &pl->chars[pl->active_char];

        switch (assist->state) {
            case STATE_ASSIST_ENTERING:
                assist->state_frame++;
                assist->x += assist->vx;
                if (assist->state_frame >= 10) {
                    /* Begin attack */
                    assist->state = STATE_ATTACK_STARTUP;
                    assist->state_frame = 0;
                    assist->vx = 0;
                }
                break;
            case STATE_ATTACK_STARTUP:
                assist->state_frame++;
                if (pl->assist_attack && assist->state_frame >= pl->assist_attack->startup_frames) {
                    assist->state = STATE_ATTACK_ACTIVE;
                    assist->state_frame = 0;
                }
                break;
            case STATE_ATTACK_ACTIVE: {
                assist->state_frame++;
                if (pl->assist_attack) {
                    int active_frames = pl->assist_attack->active_end - pl->assist_attack->active_start + 1;
                    /* Check assist attack vs opponent */
                    int opp_idx = 1 - i;
                    PlayerState *opp = &game->players[opp_idx];
                    CharacterState *opp_c = &opp->chars[opp->active_char];
                    if (pl->assist_opponent_hits[opp_idx] != pl->assist_hit_id &&
                        opp_c->wakeup_timer <= 0) {
                        /* Check if assist move is a projectile — spawn it */
                        if (pl->assist_attack->properties & MOVE_PROP_PROJECTILE) {
                            projectile_spawn(&game->projectiles, i, pl, pl->assist_attack, opp_c);
                            pl->assist_opponent_hits[opp_idx] = pl->assist_hit_id;
                        } else {
                            Hitbox hb;
                            hb.x = pl->assist_attack->x_offset;
                            hb.y = pl->assist_attack->y_offset;
                            hb.width = pl->assist_attack->width;
                            hb.height = pl->assist_attack->height;
                            hb.damage = pl->assist_attack->damage;
                            hb.hit_id = pl->assist_hit_id;
                            Hurtbox hurt;
                            hurtbox_create(opp_c, &hurt);
                            if (hitbox_check_collision(&hb, &hurt, FIXED_TO_INT(assist->x),
                                                      FIXED_TO_INT(assist->y), assist->facing)) {
                                uint32_t opp_raw = input_get_current(&game->inputs[opp_idx]);
                                int blocking_hit = is_blocking(opp_c, assist, opp_raw, pl->assist_attack->hit_type);
                                hitbox_resolve_hit(assist, opp_c, pl->assist_attack, blocking_hit,
                                                   &pl->combo, &opp->combo, &game->hitstop_frames,
                                                   game->training.active && game->training.counter_hit);
                                if (!blocking_hit) {
                                    pl->hit_confirmed = 1;
                                    trigger_screen_shake(render, pl->assist_attack->damage);
                                }
                                pl->assist_opponent_hits[opp_idx] = pl->assist_hit_id;
                            }
                        }
                    }
                    if (assist->state_frame >= active_frames) {
                        assist->state = STATE_ATTACK_RECOVERY;
                        assist->state_frame = 0;
                    }
                }
                break;
            }
            case STATE_ATTACK_RECOVERY:
                assist->state_frame++;
                if (pl->assist_attack && assist->state_frame >= pl->assist_attack->recovery_frames) {
                    assist->state = STATE_ASSIST_EXITING;
                    assist->state_frame = 0;
                    /* Walk off-screen */
                    assist->vx = FIXED_FROM_INT(-6) * assist->facing;
                }
                break;
            case STATE_ASSIST_EXITING:
                assist->state_frame++;
                assist->x += assist->vx;
                if (assist->state_frame >= 10) {
                    pl->assist_on_screen = 0;
                    pl->assist_attack = NULL;
                    assist->state = STATE_IDLE;
                    assist->state_frame = 0;
                    assist->vx = 0;
                    /* Set cooldown based on whether assist was hit */
                    pl->assist_cooldown = pl->assist_hit ? 300 : 180;
                    pl->assist_hit = 0;
                }
                break;
            default:
                break;
        }

        /* Check if assist is hit by opponent's attack */
        if (pl->assist_on_screen && assist->state != STATE_ASSIST_ENTERING) {
            int opp_idx = 1 - i;
            PlayerState *opp = &game->players[opp_idx];
            CharacterState *opp_c = &opp->chars[opp->active_char];
            if (opp_c->state == STATE_ATTACK_ACTIVE && opp->current_attack) {
                int opp_hit_idx = i;  /* assist's "player" index for hit tracking */
                if (opp->opponent_hits[opp_hit_idx] != opp->attack_hit_id) {
                    const struct MoveData *move = opp->current_attack;
                    if (!(move->properties & MOVE_PROP_PROJECTILE)) {
                        Hitbox hb;
                        hb.x = move->x_offset;
                        hb.y = move->y_offset;
                        hb.width = move->width;
                        hb.height = move->height;
                        hb.damage = move->damage;
                        hb.hit_id = opp->attack_hit_id;
                        Hurtbox hurt;
                        hurtbox_create(assist, &hurt);
                        if (hitbox_check_collision(&hb, &hurt, FIXED_TO_INT(opp_c->x),
                                                  FIXED_TO_INT(opp_c->y), opp_c->facing)) {
                            /* Assist takes 150% damage */
                            int assist_damage = (move->damage * 150) / 100;
                            assist->hp -= assist_damage;
                            assist->hit_flash = 3;
                            pl->assist_hit = 1;
                            opp->hit_confirmed = 1;
                            trigger_screen_shake(render, assist_damage);
                            render->damage_drain_delay[i][1 - pl->active_char] = 30;
                            opp->opponent_hits[opp_hit_idx] = opp->attack_hit_id;
                            /* Interrupt assist: skip to exiting */
                            assist->state = STATE_ASSIST_EXITING;
                            assist->state_frame = 0;
                            assist->vx = FIXED_FROM_INT(-6) * assist->facing;
                        }
                    }
                }
            }
        }
    }

    /* Decay screen shake */
    if (render->shake_frames > 0) render->shake_frames--;

    /* Damage drain effect: lerp display HP toward actual HP */
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            float actual = (float)game->players[i].chars[j].hp;
            float *display = &render->damage_display_hp[i][j];
            if (*display > actual) {
                if (render->damage_drain_delay[i][j] > 0) {
                    render->damage_drain_delay[i][j]--;
                } else {
                    float max_hp = (float)game->players[i].chars[j].max_hp;
                    float drain_rate = max_hp * 0.02f; /* 2% per frame */
                    *display -= drain_rate;
                    if (*display < actual) *display = actual;
                }
            } else {
                *display = actual;
                render->damage_drain_delay[i][j] = 0;
            }
        }
    }

    camera_update(render, game);
    update_combo_display(render, game);
    game->frame_count++;
}

void game_render(const GameState *game, const GameRenderState *render) {
    render_stage_bg();

    /* --- World space rendering (inside camera + screen shake) --- */
    Camera2D cam = render->camera;
    if (render->shake_frames > 0) {
        float decay = (float)render->shake_frames / (float)(render->shake_frames_max > 0 ? render->shake_frames_max : 1);
        float amp = render->shake_amplitude * decay;
        cam.target.x += ((game->frame_count % 2) * 2 - 1) * amp;
        cam.target.y += (((game->frame_count + 1) % 2) * 2 - 1) * amp * 0.4f;
    }
    BeginMode2D(cam);

    render_stage_ground();

    /* Render players: sprite if loaded, else rectangle fallback */
    for (int i = 0; i < 2; i++) {
        const PlayerState *pl = &game->players[i];
        const CharacterState *cs = &pl->chars[pl->active_char];
        int cx = FIXED_TO_INT(cs->x);
        int cy = FIXED_TO_INT(cs->y);
        if (render->sprites[i].loaded &&
            cs->current_anim >= 0 && cs->current_anim < ANIM_COUNT &&
            render->sprites[i].anims[cs->current_anim].frame_count > 0) {
            const Animation *anim = &render->sprites[i].anims[cs->current_anim];
            int draw_w = anim->frame_width;
            int draw_h = anim->frame_height;
            int draw_x = cx + cs->width / 2 - draw_w / 2;
            int draw_y = cy + cs->height - draw_h;

            /* Defender vibration during hitstop */
            if (hitbox_in_hitstop(&game->hitstop_frames) && cs->in_hitstun) {
                draw_x += (game->frame_count % 2) * 4 - 2;
            }

            sprite_draw(&render->sprites[i], cs->current_anim, cs->anim_frame,
                        draw_x, draw_y, cs->facing, draw_w, draw_h);

            /* Hit flash: bright white overlay for impact */
            if (cs->hit_flash > 0) {
                BeginBlendMode(BLEND_ADDITIVE);
                sprite_draw(&render->sprites[i], cs->current_anim, cs->anim_frame,
                            draw_x, draw_y, cs->facing, draw_w, draw_h);
                EndBlendMode();
            }

            /* Player label above sprite (world space, debug only) */
            if (render->debug_draw) {
                char plbl[4];
                snprintf(plbl, sizeof(plbl), "P%d", pl->player_id);
                DrawText(plbl, draw_x + draw_w/2 - 10, draw_y - 20, 16,
                         i == 0 ? (Color){100,255,100,255} : (Color){255,100,100,255});
            }
        } else {
            player_render(pl);
        }
    }

    /* Render assist characters when on-screen */
    for (int i = 0; i < 2; i++) {
        const PlayerState *pl = &game->players[i];
        if (!pl->assist_on_screen) continue;
        int partner_idx = 1 - pl->active_char;
        const CharacterState *assist = &pl->chars[partner_idx];
        int ax = FIXED_TO_INT(assist->x);
        int ay = FIXED_TO_INT(assist->y);
        /* Semi-transparent rectangle for assist */
        Color assist_color = (Color){
            (unsigned char)(i == 0 ? 200 : 60),
            (unsigned char)(i == 0 ? 60 : 200),
            60, 160
        };
        DrawRectangle(ax, ay, assist->width, assist->height, assist_color);
        DrawText("AST", ax + 5, ay + 5, 10, WHITE);
    }

    /* Render projectiles (world space) */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        const Projectile *proj = &game->projectiles.projectiles[i];
        if (!proj->active) continue;
        int px = FIXED_TO_INT(proj->x);
        int py = FIXED_TO_INT(proj->y);
        Color fill;
        if (proj->owner == 0) {
            int bright = 150 + proj->strength * 50;
            if (bright > 255) bright = 255;
            fill = (Color){ 0, (unsigned char)bright, (unsigned char)bright, 220 };
        } else {
            int bright = 150 + proj->strength * 50;
            if (bright > 255) bright = 255;
            fill = (Color){ (unsigned char)bright, (unsigned char)(bright / 2), 0, 220 };
        }
        DrawRectangle(px, py, proj->width, proj->height, fill);
        DrawRectangleLines(px, py, proj->width, proj->height, WHITE);
    }

    /* Special move labels (world space, above character) */
    for (int i = 0; i < 2; i++) {
        const PlayerState *pl = &game->players[i];
        const CharacterState *cs = &pl->chars[pl->active_char];
        if ((cs->state == STATE_ATTACK_STARTUP || cs->state == STATE_ATTACK_ACTIVE ||
             cs->state == STATE_ATTACK_RECOVERY) && pl->current_attack) {
            const struct MoveData *move = pl->current_attack;
            if (move->move_type == MOVE_TYPE_SPECIAL || move->move_type == MOVE_TYPE_SUPER) {
                int lx = FIXED_TO_INT(cs->x) + cs->width / 2 - 20;
                int ly = FIXED_TO_INT(cs->y) - 20;
                Color label_color = move->move_type == MOVE_TYPE_SUPER
                    ? (Color){ 255, 215, 0, 255 }
                    : (Color){ 0, 255, 255, 255 };
                DrawText(move->name, lx, ly, 10, label_color);
            }
        }
    }

    /* Debug hitbox visualization (world space) */
    if (render->debug_draw) {
        for (int i = 0; i < 2; i++) {
            const PlayerState *pl = &game->players[i];
            const CharacterState *cs = &pl->chars[pl->active_char];
            int cx = FIXED_TO_INT(cs->x);
            int cy = FIXED_TO_INT(cs->y);

            render_debug_pushbox(cx, cy, cs->width, cs->height);
            render_debug_hurtbox(cx, cy, cs->width, cs->height);

            if (cs->state == STATE_ATTACK_ACTIVE && pl->current_attack) {
                const struct MoveData *move = pl->current_attack;
                if (!(move->properties & MOVE_PROP_PROJECTILE)) {
                    int hx = cx + (cs->facing == 1 ? move->x_offset : -move->x_offset - move->width);
                    int hy = cy + move->y_offset;
                    render_debug_hitbox(hx, hy, move->width, move->height);
                }
            }
        }

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            const Projectile *proj = &game->projectiles.projectiles[i];
            if (!proj->active) continue;
            render_debug_hitbox(FIXED_TO_INT(proj->x), FIXED_TO_INT(proj->y),
                                proj->width, proj->height);
        }
    }

    EndMode2D();

    /* --- Screen space HUD (outside camera) --- */

    /* Portraits */
    render_hud_portraits();

    /* Health bars */
    for (int i = 0; i < 2; i++) {
        const CharacterState *cs = &game->players[i].chars[game->players[i].active_char];
        render_hud_health_bar(i + 1, cs->hp, cs->max_hp, cs->blue_hp,
                              render->damage_display_hp[i][game->players[i].active_char],
                              game->frame_count);
    }

    /* Partner health bars + assist cooldown */
    for (int i = 0; i < 2; i++) {
        int partner_idx = 1 - game->players[i].active_char;
        const CharacterState *partner = &game->players[i].chars[partner_idx];
        int is_ko = (partner->hp <= 0) ? 1 : 0;
        render_hud_partner_bar(i + 1, partner->hp, partner->max_hp, partner->blue_hp, is_ko);
        render_hud_assist_cooldown(i + 1, game->players[i].assist_cooldown,
                                   game->players[i].assist_hit ? 300 : 180);
    }

    /* Timer */
    render_hud_timer(game->timer_frames, game->frame_count);

    /* Meter bars */
    render_hud_meter(1, game->players[0].meter, MAX_METER, game->frame_count);
    render_hud_meter(2, game->players[1].meter, MAX_METER, game->frame_count);

    /* KO text */
    if (game->ko_timer > 0) {
        render_hud_ko(game->ko_timer, game->frame_count);
    }

    /* Combo counters */
    for (int i = 0; i < 2; i++) {
        const ComboDisplayState *cd = &render->combo_display[i];
        if (cd->fade_timer > 0 && cd->display_hit_count >= 2) {
            unsigned char alpha;
            if (game->players[i].combo.hit_count >= 2) {
                alpha = 255;
            } else {
                alpha = (unsigned char)((cd->fade_timer * 255) / 60);
            }
            render_hud_combo(i + 1, cd->display_hit_count, cd->display_damage, alpha);
        }
    }

    /* Training mode badge */
    if (game->training.active) {
        render_hud_training_badge();
    }

    /* Training menu overlay */
    if (game->training.menu_open) {
        render_training_menu(game->training.cursor, game->training.block_mode,
                             game->training.dummy_state, game->training.counter_hit,
                             game->training.hp_meter_reset,
                             game->training.remap_open, game->training.remap_cursor,
                             game->training.remap_listening,
                             &render->bindings[game->training.remap_target]);
    }
}

void game_shutdown(GameState *game, GameRenderState *render) {
    (void)game;
    sprite_unload(&render->sprites[0]);
    sprite_unload(&render->sprites[1]);
    render_shutdown();
}
