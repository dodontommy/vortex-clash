#ifndef RENDER_H
#define RENDER_H

#include "types.h"
#include "physics.h"

/* Render functions */
void render_init(void);
void render_character(const Character *c);
void render_stage(void);
void render_frame_counter(int frame);
void render_shutdown(void);

#endif /* RENDER_H */
