#ifndef RENDER_H
#define RENDER_H

#include "types.h"
#include "physics.h"

/* Render functions - using Character from physics.h */
void render_init(void);
void render_character(const Character *c);
void render_health_bar(int player_id, int hp, int max_hp, int x, int y);
void render_stage(void);
void render_frame_counter(int frame);
void render_shutdown(void);

#endif /* RENDER_H */
