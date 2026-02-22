#include "render.h"
#include "input.h"
#include <raylib.h>
#include <stdio.h>

void render_init(void) {
    /* Nothing to initialize for now */
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

void render_stage_bg(void) {
    ClearBackground((Color){40, 40, 60, 255});
}

void render_stage_ground(void) {
    /* Ground plane (world space — spans full stage width) */
    DrawRectangle(0, GROUND_Y, STAGE_WIDTH, STAGE_HEIGHT, (Color){80, 60, 40, 255});

    /* Ground line */
    DrawLine(0, GROUND_Y, STAGE_WIDTH, GROUND_Y, (Color){120, 100, 80, 255});
}

void render_frame_counter(int frame) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "Frame: %d", frame);
    DrawText(buffer, 10, 10, 20, (Color){200, 200, 200, 255});
}

void render_meter_bar(int meter, int max_meter, int x, int y) {
    int bar_width = 200;
    int bar_height = 16;
    int fill = (int)((long long)meter * bar_width / max_meter);
    if (fill < 0) fill = 0;
    if (fill > bar_width) fill = bar_width;

    /* Background (dark) */
    DrawRectangle(x, y, bar_width, bar_height, (Color){30, 30, 50, 255});
    /* Fill (blue) */
    DrawRectangle(x, y, fill, bar_height, (Color){60, 120, 255, 255});
    /* Border */
    DrawRectangleLines(x, y, bar_width, bar_height, (Color){180, 180, 200, 255});
}

void render_ko_text(void) {
    const char *text = "K.O.!";
    int font_size = 80;
    int text_width = MeasureText(text, font_size);
    DrawText(text, SCREEN_WIDTH / 2 - text_width / 2, SCREEN_HEIGHT / 2 - 40,
             font_size, (Color){255, 50, 50, 255});
}

void render_combo_counter(int hits, int damage, int x, int y, unsigned char alpha) {
    char buf[16];
    /* Large gold hit count */
    Color gold = { 255, 215, 0, alpha };
    Color white = { 255, 255, 255, alpha };
    snprintf(buf, sizeof(buf), "%d", hits);
    DrawText(buf, x, y, 40, gold);
    DrawText("HITS", x + 50, y + 10, 20, gold);
    /* Smaller damage below */
    snprintf(buf, sizeof(buf), "DMG: %d", damage);
    DrawText(buf, x, y + 45, 16, white);
}

void render_training_menu(int cursor, int block_mode, int dummy_state, int counter_hit, int hp_reset,
                          int remap_open, int remap_cursor, int remap_listening,
                          const void *bindings_ptr) {
    const InputBindings *bindings = (const InputBindings *)bindings_ptr;
    static const char *block_names[] = { "None", "All", "After 1st", "Crouch", "Stand" };
    static const char *state_names[] = { "Stand", "Crouch", "Jump" };
    static const char *labels[] = { "Block", "State", "Counter Hit", "HP Reset", "Controls", "Exit Game" };
    static const char *btn_labels[] = { "Light", "Medium", "Heavy", "Special" };

    int px = SCREEN_WIDTH - 350;
    int py = 80;
    int pw = 330;
    int ph = 300;

    /* Semi-transparent dark panel */
    DrawRectangle(px, py, pw, ph, (Color){ 20, 20, 30, 200 });
    DrawRectangleLines(px, py, pw, ph, (Color){ 100, 100, 140, 255 });

    if (remap_open) {
        /* Remap sub-menu */
        DrawText("CONTROLS (P1)", px + 70, py + 10, 20, (Color){ 255, 215, 0, 255 });

        for (int i = 0; i < REMAP_BUTTON_COUNT; i++) {
            int ry = py + 50 + i * 42;
            Color row_color = (i == remap_cursor) ? (Color){ 255, 255, 0, 255 } : (Color){ 200, 200, 200, 255 };

            DrawText(btn_labels[i], px + 20, ry, 18, row_color);

            if (remap_listening && i == remap_cursor) {
                DrawText("Press key...", px + 140, ry, 18, (Color){ 255, 100, 100, 255 });
            } else if (bindings) {
                char bind_buf[48];
                const char *kname = input_key_name(bindings->buttons[i].keyboard_key);
                const char *gname = bindings->buttons[i].gamepad_button >= 0
                    ? input_gamepad_button_name(bindings->buttons[i].gamepad_button) : "-";
                snprintf(bind_buf, sizeof(bind_buf), "Key:%s  Pad:%s", kname, gname);
                DrawText(bind_buf, px + 140, ry, 16, row_color);
            }
        }

        DrawText("A/Enter:Rebind  B/Esc:Back", px + 30, py + ph - 25, 12, (Color){ 150, 150, 150, 255 });
        return;
    }

    /* Title */
    DrawText("TRAINING MODE", px + 80, py + 10, 20, (Color){ 255, 215, 0, 255 });

    /* Menu rows */
    for (int i = 0; i < 6; i++) {
        int ry = py + 45 + i * 38;
        Color row_color = (i == cursor) ? (Color){ 255, 255, 0, 255 } : (Color){ 200, 200, 200, 255 };

        DrawText(labels[i], px + 20, ry, 18, row_color);

        if (i < 4) {
            const char *val = "";
            switch (i) {
                case 0: val = block_names[block_mode]; break;
                case 1: val = state_names[dummy_state]; break;
                case 2: val = counter_hit ? "ON" : "OFF"; break;
                case 3: val = hp_reset ? "ON" : "OFF"; break;
            }
            char row_buf[32];
            snprintf(row_buf, sizeof(row_buf), "< %s >", val);
            DrawText(row_buf, px + 170, ry, 18, row_color);
        }
    }

    /* Hint */
    DrawText("D-Pad:Nav  L/R:Change  A/Enter:Select", px + 10, py + ph - 25, 12, (Color){ 150, 150, 150, 255 });
}

void render_debug_hitbox(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, (Color){255, 0, 0, 80});
    DrawRectangleLines(x, y, w, h, (Color){255, 0, 0, 255});
}

void render_debug_hurtbox(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, (Color){0, 255, 0, 80});
    DrawRectangleLines(x, y, w, h, (Color){0, 255, 0, 255});
}

void render_debug_pushbox(int x, int y, int w, int h) {
    DrawRectangle(x, y, w, h, (Color){0, 0, 255, 80});
    DrawRectangleLines(x, y, w, h, (Color){0, 0, 255, 255});
}

void render_shutdown(void) {
    /* Nothing to shutdown */
}
