#include "physics.h"
#include <raylib.h>

/* Global player instance for Phase 2 */
Character g_player;

void physics_init(Character *c, int x, int y, int w, int h, int r, int g, int b) {
    c->x = FIXED_FROM_INT(x);
    c->y = FIXED_FROM_INT(y);
    c->vx = 0;
    c->vy = 0;
    c->width = w;
    c->height = h;
    c->color_r = r;
    c->color_g = g;
    c->color_b = b;
    c->on_ground = FALSE;
}

void physics_update(Character *c) {
    physics_apply_gravity(c);
    physics_apply_velocity(c);
    physics_ground_collision(c);
    physics_stage_bounds(c, 0, SCREEN_WIDTH);
}

void physics_apply_gravity(Character *c) {
    c->vy += GRAVITY;
}

void physics_apply_velocity(Character *c) {
    c->x += c->vx;
    c->y += c->vy;
}

void physics_ground_collision(Character *c) {
    int ground_top = GROUND_Y - c->height;
    if (c->y >= FIXED_FROM_INT(ground_top)) {
        c->y = FIXED_FROM_INT(ground_top);
        c->vy = 0;
        c->on_ground = TRUE;
    } else {
        c->on_ground = FALSE;
    }
}

void physics_stage_bounds(Character *c, int stage_left, int stage_right) {
    if (c->x < FIXED_FROM_INT(stage_left)) {
        c->x = FIXED_FROM_INT(stage_left);
        c->vx = 0;
    }
    if (c->x > FIXED_FROM_INT(stage_right - c->width)) {
        c->x = FIXED_FROM_INT(stage_right - c->width);
        c->vx = 0;
    }
}
