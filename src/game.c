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
    memset(&game->training, 0, sizeof(TrainingState));

    /* Render state */
    sprite_load(&render->sprites[0], "ryker");
    sprite_load(&render->sprites[1], "ryker");
    render->running = TRUE;
    render->debug_draw = FALSE;
    memset(render->combo_display, 0, sizeof(render->combo_display));
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

    /* KO check: did either player's HP drop to 0? (skip in training mode) */
    if (game->ko_winner < 0 && !game->training.active) {
        CharacterState *c1 = &game->players[0].chars[game->players[0].active_char];
        CharacterState *c2 = &game->players[1].chars[game->players[1].active_char];
        if (c1->hp <= 0) {
            game->ko_winner = 1;
            game->ko_timer = KO_FREEZE_FRAMES;
        } else if (c2->hp <= 0) {
            game->ko_winner = 0;
            game->ko_timer = KO_FREEZE_FRAMES;
        }
    }

    /* Decay screen shake */
    if (render->shake_frames > 0) render->shake_frames--;

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
    const CharacterState *p1c = &game->players[0].chars[game->players[0].active_char];
    const CharacterState *p2c = &game->players[1].chars[game->players[1].active_char];
    render_health_bar(1, p1c->hp, p1c->max_hp, 50, 30);
    render_health_bar(2, p2c->hp, p2c->max_hp, SCREEN_WIDTH - 450, 30);
    render_meter_bar(game->players[0].meter, MAX_METER, 50, 65);
    render_meter_bar(game->players[1].meter, MAX_METER, SCREEN_WIDTH - 250, 65);
    render_frame_counter(game->frame_count);

    /* KO text */
    if (game->ko_timer > 0) {
        render_ko_text();
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
            int cx = (i == 0) ? 60 : SCREEN_WIDTH - 240;
            render_combo_counter(cd->display_hit_count, cd->display_damage, cx, 80, alpha);
        }
    }

    /* Training mode indicator */
    if (game->training.active) {
        DrawText("TRAINING", SCREEN_WIDTH / 2 - 50, 10, 20, (Color){ 255, 215, 0, 200 });
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
