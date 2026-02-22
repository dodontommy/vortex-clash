#ifndef RENDER_H
#define RENDER_H

#include "types.h"

/* Render functions */
void render_init(void);
void render_health_bar(int player_id, int hp, int max_hp, int x, int y);
void render_stage_bg(void);
void render_stage_ground(void);
void render_frame_counter(int frame);
void render_shutdown(void);

/* Meter bar */
void render_meter_bar(int meter, int max_meter, int x, int y);

/* KO text */
void render_ko_text(void);

/* Combo counter + training menu */
void render_combo_counter(int hits, int damage, int x, int y, unsigned char alpha);
void render_training_menu(int cursor, int block_mode, int dummy_state, int counter_hit, int hp_reset,
                          int remap_open, int remap_cursor, int remap_listening,
                          const void *bindings);

/* Debug visualization */
void render_debug_hitbox(int x, int y, int w, int h);
void render_debug_hurtbox(int x, int y, int w, int h);
void render_debug_pushbox(int x, int y, int w, int h);

#endif /* RENDER_H */
