#include "game.h"
#include "player.h"
#include "render.h"
#include "hitbox.h"
#include <raylib.h>

static void check_attack_hit(PlayerState *attacker, PlayerState *defender, uint32_t defender_input) {
    CharacterState *a = &attacker->chars[attacker->active_char];
    CharacterState *d = &defender->chars[defender->active_char];
    
    /* Only check if attacker is in active frames of attack */
    if (a->state != STATE_ATTACK_ACTIVE || !attacker->current_attack)
        return;
    
    /* Check if already hit this opponent in this attack */
    int opp_idx = defender->player_id - 1;
    if (attacker->opponent_hits[opp_idx] == attacker->attack_hit_id)
        return;
    
    /* Create hitbox */
    const AttackMove *move = attacker->current_attack;
    Hitbox hb;
    hb.x = move->x_offset;
    hb.y = move->y_offset;
    hb.width = move->width;
    hb.height = move->height;
    hb.damage = move->damage;
    hb.hit_id = attacker->attack_hit_id;
    
    /* Create defender hurtbox */
    Hurtbox hurt;
    hurtbox_create(d, &hurt);
    
    /* Check collision */
    if (hitbox_check_collision(&hb, &hurt, FIXED_TO_INT(a->x), FIXED_TO_INT(a->y), a->facing)) {
        /* Check if blocking */
        int blocking = is_blocking(d, a, defender_input);
        
        /* Apply hit */
        hitbox_resolve_hit(a, d, move, blocking);
        
        /* Mark as hit */
        attacker->opponent_hits[opp_idx] = attacker->attack_hit_id;
    }
}

void game_init(GameState *game) {
    player_init(&game->p1, 1, 200, GROUND_Y - 80);
    player_init(&game->p2, 2, 900, GROUND_Y - 80);
    input_init(&game->p1_input);
    input_init(&game->p2_input);
    game->frame_count = 0;
    game->running = TRUE;
    render_init();
    hitbox_init();
}

void game_update(GameState *game) {
    /* Update hitstop */
    hitbox_update_hitstop();
    if (hitbox_in_hitstop())
        return;  /* Skip update during hitstop */
    
    /* Poll inputs */
    uint32_t p1_raw = input_poll(1, INPUT_SOURCE_KEYBOARD);
    uint32_t p2_raw = input_poll(2, INPUT_SOURCE_KEYBOARD);
    
    /* Update input buffers */
    input_update(&game->p1_input, p1_raw);
    input_update(&game->p2_input, p2_raw);
    
    /* Use buffered input for players */
    player_update(&game->p1, input_get_current(&game->p1_input));
    player_update(&game->p2, input_get_current(&game->p2_input));
    player_update_facing(&game->p1, &game->p2);
    player_resolve_collisions(&game->p1, &game->p2);
    
    /* Check for attack hits */
    check_attack_hit(&game->p1, &game->p2, p2_raw);
    check_attack_hit(&game->p2, &game->p1, p1_raw);
    
    game->frame_count++;
}

void game_render(const GameState *game) {
    render_stage();
    
    /* Render health bars */
    CharacterState *p1 = &game->p1.chars[game->p1.active_char];
    CharacterState *p2 = &game->p2.chars[game->p2.active_char];
    render_health_bar(1, p1->hp, p1->max_hp, 50, 30);
    render_health_bar(2, p2->hp, p2->max_hp, SCREEN_WIDTH - 450, 30);
    
    player_render(&game->p1);
    player_render(&game->p2);
    render_frame_counter(game->frame_count);
}

void game_shutdown(GameState *game) {
    render_shutdown();
}