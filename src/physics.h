#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"

/* Character state for Phase 2 */
typedef struct {
    fixed_t x, y;           /* Position */
    fixed_t vx, vy;         /* Velocity */
    int width, height;      /* Dimensions */
    int color_r, color_g, color_b;  /* RGB color */
    bool_t on_ground;
} Character;

/* Physics constants */
#define GRAVITY (FIXED_FROM_INT(1) / 2)  /* ~0.5 pixels/frame^2 */
#define JUMP_VELOCITY FIXED_FROM_INT(-12)
#define MOVE_SPEED FIXED_FROM_INT(5)
#define GROUND_Y 600  /* Y position of ground in screen coords */

/* Physics functions */
void physics_init(Character *c, int x, int y, int w, int h, int r, int g, int b);
void physics_update(Character *c);
void physics_apply_gravity(Character *c);
void physics_apply_velocity(Character *c);
void physics_ground_collision(Character *c);
void physics_stage_bounds(Character *c, int stage_left, int stage_right);

#endif /* PHYSICS_H */
