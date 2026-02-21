#include "render.h"
#include <raylib.h>
#include <stdio.h>

void render_init(void) {
    /* Nothing to initialize for now */
}

void render_character(const Character *c) {
    int x = FIXED_TO_INT(c->x);
    int y = FIXED_TO_INT(c->y);
    Color color = (Color){c->color_r, c->color_g, c->color_b, 255};
    DrawRectangle(x, y, c->width, c->height, color);
}

void render_stage(void) {
    /* Background color */
    ClearBackground((Color){40, 40, 60, 255});

    /* Ground plane */
    DrawRectangle(0, GROUND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - GROUND_Y, (Color){80, 60, 40, 255});

    /* Ground line */
    DrawLine(0, GROUND_Y, SCREEN_WIDTH, GROUND_Y, (Color){120, 100, 80, 255});
}

void render_frame_counter(int frame) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Frame: %d", frame);
    DrawText(buffer, 10, 10, 20, (Color){200, 200, 200, 255});
}

void render_shutdown(void) {
    /* Nothing to shutdown */
}
