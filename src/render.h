#ifndef RENDER_H
#define RENDER_H

#include "types.h"

/* Forward declarations */
struct GameState;
struct GameRenderState;
struct PlayerState;
struct CharacterState;

/* Render lifecycle */
void render_init(void);
void render_shutdown(void);

/* Stage rendering */
void render_stage_bg(void);
void render_stage_ground(void);

/* New MvC3-style HUD functions */
void render_hud_health_bar(int player_id, int hp, int max_hp, int blue_hp,
                           float damage_hp, int frame_count);
void render_hud_partner_bar(int player_id, int hp, int max_hp, int blue_hp,
                            int is_ko);
void render_hud_assist_cooldown(int player_id, int cooldown, int max_cooldown);
void render_hud_meter(int player_id, int meter, int max_meter, int frame_count);
void render_hud_timer(int timer_frames, int frame_count);
void render_hud_portraits(void);
void render_hud_combo(int player_id, int hits, int damage, unsigned char alpha);
void render_hud_ko(int ko_timer, int frame_count);
void render_hud_training_badge(void);

/* Training menu (kept as-is) */
void render_training_menu(int cursor, int block_mode, int dummy_state, int counter_hit, int hp_reset,
                          int remap_open, int remap_cursor, int remap_listening,
                          const void *bindings);

/* Debug visualization */
void render_debug_hitbox(int x, int y, int w, int h);
void render_debug_hurtbox(int x, int y, int w, int h);
void render_debug_pushbox(int x, int y, int w, int h);

#endif /* RENDER_H */
