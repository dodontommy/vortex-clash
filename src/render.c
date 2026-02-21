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

void render_health_bar(int player_id, int hp, int max_hp, int x, int y) {
    int bar_width = 400;
    int bar_height = 30;
    int hp_width = (int)((long long)hp * bar_width / max_hp);
    if (hp_width < 0) hp_width = 0;
    
    /* Background (red/damage) */
    DrawRectangle(x, y, bar_width, bar_height, (Color){200, 50, 50, 255});
    
    /* HP (green) */
    DrawRectangle(x, y, hp_width, bar_height, (Color){50, 200, 50, 255});
    
    /* Border */
    DrawRectangleLines(x, y, bar_width, bar_height, (Color){255, 255, 255, 255});
    
    /* Player label */
    char label[8];
    snprintf(label, sizeof(label), "P%d", player_id);
    DrawText(label, x + 5, y + 5, 20, (Color){255, 255, 255, 255});
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
