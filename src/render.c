#include "render.h"
#include "game.h"
#include "input.h"
#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <math.h>

/* ===== Color Palette ===== */
#define HUD_BG_DARK       (Color){15, 15, 25, 230}
#define HUD_BG_MID        (Color){30, 30, 50, 230}
#define HUD_CHROME        (Color){60, 60, 80, 255}
#define HUD_CHROME_LIGHT  (Color){100, 100, 130, 255}
#define P1_HP_COLOR       (Color){0, 200, 220, 255}
#define P1_HP_BRIGHT      (Color){80, 255, 255, 255}
#define P1_ACCENT         (Color){0, 180, 255, 255}
#define P2_HP_COLOR       (Color){220, 40, 80, 255}
#define P2_HP_BRIGHT      (Color){255, 100, 140, 255}
#define P2_ACCENT         (Color){255, 40, 120, 255}
#define BLUE_HP_COLOR     (Color){80, 120, 200, 180}
#define DAMAGE_RED        (Color){180, 30, 30, 200}
#define METER_FILL        (Color){255, 200, 0, 255}
#define METER_BRIGHT      (Color){255, 240, 100, 255}
#define COMBO_GOLD        (Color){255, 215, 0, 255}
#define KO_RED            (Color){255, 40, 40, 255}

/* ===== Custom Font ===== */
static Font hud_font = {0};
static int hud_font_loaded = 0;

/* Draw text with the custom HUD font. Falls back to default if not loaded. */
static void hud_draw_text(const char *text, float x, float y, float size, Color color) {
    if (hud_font_loaded) {
        DrawTextEx(hud_font, text, (Vector2){x, y}, size, 1.0f, color);
    } else {
        DrawText(text, (int)x, (int)y, (int)size, color);
    }
}

static float hud_measure_text(const char *text, float size) {
    if (hud_font_loaded) {
        Vector2 v = MeasureTextEx(hud_font, text, size, 1.0f);
        return v.x;
    }
    return (float)MeasureText(text, (int)size);
}

/* ===== Slanted Rect Helpers (RL_TRIANGLES) ===== */

static void draw_slanted_rect(float x, float y, float w, float h, float slant, Color c) {
    float tlx, tly, trx, try_, blx, bly, brx, bry;
    if (slant >= 0) {
        tlx = x;           tly = y;
        trx = x + w - slant; try_ = y;
        brx = x + w;       bry = y + h;
        blx = x;           bly = y + h;
    } else {
        float s = -slant;
        tlx = x + s;       tly = y;
        trx = x + w;       try_ = y;
        brx = x + w;       bry = y + h;
        blx = x;           bly = y + h;
    }
    rlBegin(RL_TRIANGLES);
        rlColor4ub(c.r, c.g, c.b, c.a);
        rlVertex2f(tlx, tly); rlVertex2f(blx, bly); rlVertex2f(brx, bry);
        rlVertex2f(tlx, tly); rlVertex2f(brx, bry); rlVertex2f(trx, try_);
    rlEnd();
}

static void draw_slanted_rect_gradient_v(float x, float y, float w, float h,
                                         float slant, Color top, Color bot) {
    float tlx, tly, trx, try_, blx, bly, brx, bry;
    if (slant >= 0) {
        tlx = x;           tly = y;
        trx = x + w - slant; try_ = y;
        brx = x + w;       bry = y + h;
        blx = x;           bly = y + h;
    } else {
        float s = -slant;
        tlx = x + s;       tly = y;
        trx = x + w;       try_ = y;
        brx = x + w;       bry = y + h;
        blx = x;           bly = y + h;
    }
    rlBegin(RL_TRIANGLES);
        rlColor4ub(top.r, top.g, top.b, top.a); rlVertex2f(tlx, tly);
        rlColor4ub(bot.r, bot.g, bot.b, bot.a); rlVertex2f(blx, bly);
        rlColor4ub(bot.r, bot.g, bot.b, bot.a); rlVertex2f(brx, bry);
        rlColor4ub(top.r, top.g, top.b, top.a); rlVertex2f(tlx, tly);
        rlColor4ub(bot.r, bot.g, bot.b, bot.a); rlVertex2f(brx, bry);
        rlColor4ub(top.r, top.g, top.b, top.a); rlVertex2f(trx, try_);
    rlEnd();
}

static void draw_slanted_rect_lines(float x, float y, float w, float h,
                                    float slant, Color color) {
    Vector2 tl, tr, br, bl;
    if (slant >= 0) {
        tl = (Vector2){x, y};
        tr = (Vector2){x + w - slant, y};
        br = (Vector2){x + w, y + h};
        bl = (Vector2){x, y + h};
    } else {
        float s = -slant;
        tl = (Vector2){x + s, y};
        tr = (Vector2){x + w, y};
        br = (Vector2){x + w, y + h};
        bl = (Vector2){x, y + h};
    }
    DrawLineV(tl, tr, color);
    DrawLineV(tr, br, color);
    DrawLineV(br, bl, color);
    DrawLineV(bl, tl, color);
}

void render_init(void) {
    /* Load custom HUD font */
    if (FileExists("assets/fonts/Rajdhani-Bold.ttf")) {
        hud_font = LoadFontEx("assets/fonts/Rajdhani-Bold.ttf", 64, NULL, 0);
        SetTextureFilter(hud_font.texture, TEXTURE_FILTER_BILINEAR);
        hud_font_loaded = 1;
    }
}

/* ===== Health Bar ===== */
void render_hud_health_bar(int player_id, int hp, int max_hp, int blue_hp,
                           float damage_hp, int frame_count) {
    float bx = (player_id == 1) ? 85.0f : 735.0f;
    float by = 12.0f;
    float bw = 460.0f;
    float bh = 32.0f;
    float slant = (player_id == 1) ? 15.0f : -15.0f;

    Color hp_color = (player_id == 1) ? P1_HP_COLOR : P2_HP_COLOR;
    Color hp_bright = (player_id == 1) ? P1_HP_BRIGHT : P2_HP_BRIGHT;

    float hp_frac = (max_hp > 0) ? (float)hp / (float)max_hp : 0.0f;
    if (hp_frac < 0.0f) hp_frac = 0.0f;
    if (hp_frac > 1.0f) hp_frac = 1.0f;

    float dmg_frac = (max_hp > 0) ? damage_hp / (float)max_hp : 0.0f;
    if (dmg_frac < 0.0f) dmg_frac = 0.0f;
    if (dmg_frac > 1.0f) dmg_frac = 1.0f;

    float blue_frac = (max_hp > 0) ? (float)blue_hp / (float)max_hp : 0.0f;
    if (blue_frac < 0.0f) blue_frac = 0.0f;

    /* Low HP pulse */
    unsigned char pulse_alpha = 255;
    if (hp_frac < 0.25f) {
        float pulse = sinf((float)frame_count * 0.15f) * 0.5f + 0.5f;
        pulse_alpha = (unsigned char)(155 + (int)(pulse * 100));
    }

    /* Layer 1: Slanted dark background (only this + border get the slant) */
    draw_slanted_rect(bx, by, bw, bh, slant, HUD_BG_DARK);

    /* Inner fill layers use slanted rects with the SAME slant as background.
       For P1 (slant>0), the slant is on the right edge. Partial fills that don't
       reach the right edge use slant=0; full-width fills use the full slant.
       For P2 (slant<0), the slant is on the left edge — mirrored logic.
       This ensures fills tile flush against the background with no gaps. */

    if (player_id == 1) {
        /* P1 fills left→right. Right edge has the slant. */
        float dmg_w = dmg_frac * bw;
        float fill_w = hp_frac * bw;
        float blue_start = fill_w;
        float blue_w = blue_frac * bw;

        /* Use full slant only when fill reaches the right edge (frac >= ~0.97) */
        float dmg_slant = (dmg_frac > 0.97f) ? slant : 0;
        float fill_slant = (hp_frac > 0.97f) ? slant : 0;

        if (dmg_frac > hp_frac) {
            draw_slanted_rect(bx, by, dmg_w, bh, dmg_slant, DAMAGE_RED);
        }
        if (blue_frac > 0.001f) {
            draw_slanted_rect(bx + blue_start, by, blue_w, bh, 0, BLUE_HP_COLOR);
        }
        if (hp_frac > 0.001f) {
            Color bot = hp_color;  bot.a = pulse_alpha;
            Color top = hp_bright; top.a = pulse_alpha;
            draw_slanted_rect_gradient_v(bx, by, fill_w, bh, fill_slant, top, bot);
        }
    } else {
        /* P2 fills right→left. Left edge has the slant. */
        float right_edge = bx + bw;
        float dmg_w = dmg_frac * bw;
        float fill_w = hp_frac * bw;
        float blue_w = blue_frac * bw;

        /* Use full slant only when fill reaches the left edge (frac >= ~0.97) */
        float dmg_slant = (dmg_frac > 0.97f) ? slant : 0;
        float fill_slant = (hp_frac > 0.97f) ? slant : 0;

        if (dmg_frac > hp_frac) {
            draw_slanted_rect(right_edge - dmg_w, by, dmg_w, bh, dmg_slant, DAMAGE_RED);
        }
        if (blue_frac > 0.001f) {
            draw_slanted_rect(right_edge - fill_w - blue_w, by, blue_w, bh, 0, BLUE_HP_COLOR);
        }
        if (hp_frac > 0.001f) {
            Color bot = hp_color;  bot.a = pulse_alpha;
            Color top = hp_bright; top.a = pulse_alpha;
            draw_slanted_rect_gradient_v(right_edge - fill_w, by, fill_w, bh,
                                         fill_slant, top, bot);
        }
    }

    /* Slanted border + shine */
    draw_slanted_rect_lines(bx, by, bw, bh, slant, HUD_CHROME);
    if (slant >= 0) {
        DrawLineEx((Vector2){bx + 1, by + 1}, (Vector2){bx + bw - slant - 1, by + 1},
                   1.0f, (Color){255, 255, 255, 50});
    } else {
        float s = -slant;
        DrawLineEx((Vector2){bx + s + 1, by + 1}, (Vector2){bx + bw - 1, by + 1},
                   1.0f, (Color){255, 255, 255, 50});
    }
}

/* ===== Partner Health Bar ===== */
void render_hud_partner_bar(int player_id, int hp, int max_hp, int blue_hp,
                            int is_ko) {
    float bx = (player_id == 1) ? 85.0f : 995.0f;
    float by = 50.0f;
    float bw = 200.0f;
    float bh = 10.0f;
    Color fill_color = (player_id == 1) ?
        (Color){0, 160, 180, 220} : (Color){180, 40, 70, 220};

    /* "PARTNER" label */
    Color label_color = (player_id == 1) ? P1_ACCENT : P2_ACCENT;
    label_color.a = 180;
    if (player_id == 1) {
        hud_draw_text("PARTNER", bx, by + bh + 1, 12, label_color);
    } else {
        float lw = hud_measure_text("PARTNER", 12);
        hud_draw_text("PARTNER", bx + bw - lw, by + bh + 1, 12, label_color);
    }

    /* Background */
    DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, HUD_BG_DARK);

    if (!is_ko) {
        /* Compute fill widths from clamped HP values */
        int total_hp = hp + blue_hp;
        if (total_hp > max_hp) total_hp = max_hp;
        float total_frac = (max_hp > 0) ? (float)total_hp / (float)max_hp : 0.0f;
        float hp_frac = (max_hp > 0) ? (float)hp / (float)max_hp : 0.0f;
        if (total_frac > 1.0f) total_frac = 1.0f;
        if (hp_frac < 0.0f) hp_frac = 0.0f;
        if (hp_frac > 1.0f) hp_frac = 1.0f;

        int fill_w = (int)(hp_frac * bw);
        int total_w = (int)(total_frac * bw);

        /* Blue segment behind HP fill */
        if (total_w > fill_w) {
            DrawRectangle((int)bx + fill_w, (int)by, total_w - fill_w, (int)bh, BLUE_HP_COLOR);
        }
        /* HP fill on top */
        DrawRectangle((int)bx, (int)by, fill_w, (int)bh, fill_color);
    } else {
        /* KO'd partner */
        DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, (Color){60, 20, 20, 200});
        int cx = (int)(bx + bw / 2);
        int cy = (int)(by + bh / 2);
        DrawLine(cx - 6, cy - 4, cx + 6, cy + 4, KO_RED);
        DrawLine(cx + 6, cy - 4, cx - 6, cy + 4, KO_RED);
    }

    DrawRectangleLines((int)bx, (int)by, (int)bw, (int)bh, HUD_CHROME);
}

/* ===== Assist Cooldown ===== */
void render_hud_assist_cooldown(int player_id, int cooldown, int max_cooldown) {
    float cx = (player_id == 1) ? 295.0f : 985.0f;
    float cy = 55.0f;
    float r = 8.0f;
    Color accent = (player_id == 1) ? P1_ACCENT : P2_ACCENT;

    DrawCircle((int)cx, (int)cy, r, HUD_BG_DARK);
    if (cooldown <= 0) {
        DrawCircle((int)cx, (int)cy, r - 1, accent);
    } else if (max_cooldown > 0) {
        float frac = 1.0f - (float)cooldown / (float)max_cooldown;
        float end_angle = frac * 360.0f;
        Color dim = accent; dim.a = 160;
        DrawCircleSector((Vector2){cx, cy}, r - 1, -90.0f, -90.0f + end_angle, 16, dim);
    }
    DrawCircleLines((int)cx, (int)cy, r, HUD_CHROME);
}

/* ===== Meter Bar (5 segments) ===== */
void render_hud_meter(int player_id, int meter, int max_meter, int frame_count) {
    (void)frame_count;
    float bx = (player_id == 1) ? 85.0f : 915.0f;
    float by = 678.0f;
    float total_w = 280.0f;
    float bh = 20.0f;
    int segments = 5;
    float gap = 2.0f;
    float seg_w = (total_w - gap * (segments - 1)) / segments;
    float slant = (player_id == 1) ? 8.0f : -8.0f;

    int meter_per_seg = max_meter / segments;

    for (int i = 0; i < segments; i++) {
        float sx = bx + i * (seg_w + gap);
        int seg_start = i * meter_per_seg;
        int seg_end = (i + 1) * meter_per_seg;

        float seg_slant = 0.0f;
        if (player_id == 1 && i == segments - 1) seg_slant = slant;
        if (player_id == 2 && i == 0) seg_slant = slant;

        /* Segment background */
        draw_slanted_rect(sx, by, seg_w, bh, seg_slant, (Color){25, 25, 40, 200});

        if (meter > seg_start) {
            float fill_frac = (meter >= seg_end) ? 1.0f
                : (float)(meter - seg_start) / (float)meter_per_seg;
            float fill_w = fill_frac * seg_w;

            /* Gold gradient fill */
            draw_slanted_rect_gradient_v(sx, by, fill_w, bh,
                                         seg_slant * fill_frac, METER_BRIGHT, METER_FILL);
        }

        /* Full segments get a brighter gold border, partial/empty get chrome */
        if (meter >= seg_end) {
            draw_slanted_rect_lines(sx, by, seg_w, bh, seg_slant, (Color){255, 220, 100, 200});
        } else {
            draw_slanted_rect_lines(sx, by, seg_w, bh, seg_slant, HUD_CHROME);
        }
    }

    draw_slanted_rect_lines(bx, by, total_w, bh, slant, HUD_CHROME_LIGHT);

    /* Meter label */
    Color label_c = (player_id == 1) ? P1_ACCENT : P2_ACCENT;
    label_c.a = 180;
    if (player_id == 1) {
        hud_draw_text("METER", bx + total_w + 8, by + 2, 16, label_c);
    } else {
        float lw = hud_measure_text("METER", 16);
        hud_draw_text("METER", bx - lw - 8, by + 2, 16, label_c);
    }
}

/* ===== Timer ===== */
void render_hud_timer(int timer_frames, int frame_count) {
    int seconds = timer_frames / 60;
    if (seconds < 0) seconds = 0;
    if (seconds > 99) seconds = 99;

    float tx = 590.0f;
    float ty = 4.0f;
    float tw = 100.0f;
    float th = 54.0f;
    float trap_slant = 8.0f;

    /* Trapezoid background */
    rlBegin(RL_TRIANGLES);
        rlColor4ub(15, 15, 25, 240);
        rlVertex2f(tx + trap_slant, ty);
        rlVertex2f(tx, ty + th);
        rlVertex2f(tx + tw, ty + th);
        rlVertex2f(tx + trap_slant, ty);
        rlVertex2f(tx + tw, ty + th);
        rlVertex2f(tx + tw - trap_slant, ty);
    rlEnd();

    /* Trapezoid border */
    DrawLineV((Vector2){tx + trap_slant, ty}, (Vector2){tx + tw - trap_slant, ty}, HUD_CHROME);
    DrawLineV((Vector2){tx + tw - trap_slant, ty}, (Vector2){tx + tw, ty + th}, HUD_CHROME);
    DrawLineV((Vector2){tx + tw, ty + th}, (Vector2){tx, ty + th}, HUD_CHROME);
    DrawLineV((Vector2){tx, ty + th}, (Vector2){tx + trap_slant, ty}, HUD_CHROME);

    /* Accent divider */
    float mid = tx + tw / 2.0f;
    DrawLineEx((Vector2){tx + 4, ty + th - 2}, (Vector2){mid, ty + th - 2}, 2.0f, P1_ACCENT);
    DrawLineEx((Vector2){mid, ty + th - 2}, (Vector2){tx + tw - 4, ty + th - 2}, 2.0f, P2_ACCENT);

    /* Timer digits */
    char timer_buf[4];
    snprintf(timer_buf, sizeof(timer_buf), "%02d", seconds);
    float font_size = 42;
    float text_w = hud_measure_text(timer_buf, font_size);

    Color timer_color = WHITE;
    if (seconds < 10) {
        float pulse = sinf((float)frame_count * 0.2f) * 0.4f + 0.6f;
        unsigned char ra = (unsigned char)(pulse * 255);
        timer_color = (Color){255, 60, 60, ra};
    }

    hud_draw_text(timer_buf, tx + tw / 2 - text_w / 2, ty + 6, font_size, timer_color);
}

/* ===== Portraits ===== */
void render_hud_portraits(void) {
    /* P1 */
    DrawRectangle(10, 8, 70, 70, HUD_BG_MID);
    DrawRectangleLines(10, 8, 70, 70, HUD_CHROME);
    DrawTriangle((Vector2){80, 8}, (Vector2){80, 24}, (Vector2){68, 8}, P1_ACCENT);
    float p1w = hud_measure_text("P1", 28);
    hud_draw_text("P1", 45 - p1w / 2, 26, 28, P1_ACCENT);

    /* P2 */
    DrawRectangle(1200, 8, 70, 70, HUD_BG_MID);
    DrawRectangleLines(1200, 8, 70, 70, HUD_CHROME);
    DrawTriangle((Vector2){1200, 8}, (Vector2){1200, 24}, (Vector2){1212, 8}, P2_ACCENT);
    float p2w = hud_measure_text("P2", 28);
    hud_draw_text("P2", 1235 - p2w / 2, 26, 28, P2_ACCENT);
}

/* ===== Combo Counter ===== */
void render_hud_combo(int player_id, int hits, int damage, unsigned char alpha) {
    float px = (player_id == 1) ? 60.0f : 1060.0f;
    float py = 200.0f;
    float pw = 160.0f;
    float ph = 90.0f;

    DrawRectangle((int)px, (int)py, (int)pw, (int)ph,
                  (Color){15, 15, 25, (unsigned char)(alpha * 0.8f)});

    Color accent = (player_id == 1) ? P1_ACCENT : P2_ACCENT;
    accent.a = alpha;
    if (player_id == 1) {
        DrawRectangle((int)px, (int)py, 3, (int)ph, accent);
    } else {
        DrawRectangle((int)(px + pw - 3), (int)py, 3, (int)ph, accent);
    }

    char buf[16];
    Color gold = COMBO_GOLD; gold.a = alpha;
    Color white = {255, 255, 255, alpha};

    float text_x = px + 12;

    /* Large hit count */
    snprintf(buf, sizeof(buf), "%d", hits);
    hud_draw_text(buf, text_x, py + 6, 52, gold);

    /* "HITS" label */
    float num_w = hud_measure_text(buf, 52);
    hud_draw_text("HITS", text_x + num_w + 4, py + 26, 22, gold);

    /* Damage below */
    snprintf(buf, sizeof(buf), "DMG %d", damage);
    hud_draw_text(buf, text_x, py + 62, 18, white);
}

/* ===== KO Text ===== */
void render_hud_ko(int ko_timer, int frame_count) {
    (void)frame_count;
    const char *text = "K.O.!";
    float font_size = 96;

    /* Scale animation over first 15 frames */
    int elapsed = KO_FREEZE_FRAMES - ko_timer;
    float scale = 1.0f;
    if (elapsed < 15) {
        scale = 0.5f + 0.5f * ((float)elapsed / 15.0f);
    }

    float scaled_size = font_size * scale;
    float scaled_w = hud_measure_text(text, scaled_size);
    float sx = SCREEN_WIDTH / 2.0f - scaled_w / 2.0f;
    float sy = SCREEN_HEIGHT / 2.0f - scaled_size / 2.0f;

    /* Shadow → glow → main → shine */
    hud_draw_text(text, sx + 3, sy + 3, scaled_size, (Color){0, 0, 0, 180});
    BeginBlendMode(BLEND_ADDITIVE);
    hud_draw_text(text, sx, sy, scaled_size, (Color){255, 80, 80, 100});
    EndBlendMode();
    hud_draw_text(text, sx, sy, scaled_size, KO_RED);
    hud_draw_text(text, sx, sy - 1, scaled_size, (Color){255, 200, 200, 60});
}

/* ===== Training Badge ===== */
void render_hud_training_badge(void) {
    float px = SCREEN_WIDTH / 2.0f - 55.0f;
    float py = 62.0f;
    float pw = 110.0f;
    float ph = 24.0f;

    DrawRectangleRounded((Rectangle){px, py, pw, ph}, 0.5f, 8, (Color){20, 20, 30, 220});
    DrawRectangleRoundedLinesEx((Rectangle){px, py, pw, ph}, 0.5f, 8, 2.0f,
                                (Color){255, 200, 0, 200});
    float tw = hud_measure_text("TRAINING", 16);
    hud_draw_text("TRAINING", px + pw / 2 - tw / 2, py + 4, 16, (Color){255, 215, 0, 230});
}

/* ===== Stage Rendering ===== */
void render_stage_bg(void) {
    ClearBackground((Color){40, 40, 60, 255});
}

void render_stage_ground(void) {
    DrawRectangle(0, GROUND_Y, STAGE_WIDTH, STAGE_HEIGHT, (Color){80, 60, 40, 255});
    DrawLine(0, GROUND_Y, STAGE_WIDTH, GROUND_Y, (Color){120, 100, 80, 255});
}

/* ===== Training Menu ===== */
void render_training_menu(int cursor, int block_mode, int dummy_state, int counter_hit, int hp_reset,
                          int remap_open, int remap_cursor, int remap_listening,
                          const void *bindings_ptr) {
    const InputBindings *bindings = (const InputBindings *)bindings_ptr;
    static const char *block_names[] = { "None", "All", "After 1st", "Crouch", "Stand" };
    static const char *state_names[] = { "Stand", "Crouch", "Jump" };
    static const char *labels[] = { "Block", "State", "Counter Hit", "HP Reset", "Controls", "Exit Game" };
    static const char *btn_labels[] = { "Light", "Medium", "Heavy", "Special", "Assist" };

    int px = SCREEN_WIDTH - 350;
    int py = 80;
    int pw = 330;
    int ph = 300;

    DrawRectangle(px, py, pw, ph, (Color){ 20, 20, 30, 200 });
    DrawRectangleLines(px, py, pw, ph, (Color){ 100, 100, 140, 255 });

    if (remap_open) {
        hud_draw_text("CONTROLS (P1)", (float)(px + 60), (float)(py + 10), 22, (Color){ 255, 215, 0, 255 });
        for (int i = 0; i < REMAP_BUTTON_COUNT; i++) {
            int ry = py + 50 + i * 42;
            Color row_color = (i == remap_cursor) ? (Color){ 255, 255, 0, 255 } : (Color){ 200, 200, 200, 255 };
            hud_draw_text(btn_labels[i], (float)(px + 20), (float)ry, 20, row_color);
            if (remap_listening && i == remap_cursor) {
                hud_draw_text("Press key...", (float)(px + 140), (float)ry, 20, (Color){ 255, 100, 100, 255 });
            } else if (bindings) {
                char bind_buf[48];
                const char *kname = input_key_name(bindings->buttons[i].keyboard_key);
                const char *gname = bindings->buttons[i].gamepad_button >= 0
                    ? input_gamepad_button_name(bindings->buttons[i].gamepad_button) : "-";
                snprintf(bind_buf, sizeof(bind_buf), "Key:%s  Pad:%s", kname, gname);
                hud_draw_text(bind_buf, (float)(px + 140), (float)ry, 16, row_color);
            }
        }
        hud_draw_text("A/Enter:Rebind  B/Esc:Back", (float)(px + 30), (float)(py + ph - 25), 14, (Color){ 150, 150, 150, 255 });
        return;
    }

    hud_draw_text("TRAINING MODE", (float)(px + 70), (float)(py + 10), 24, (Color){ 255, 215, 0, 255 });
    for (int i = 0; i < 6; i++) {
        int ry = py + 45 + i * 38;
        Color row_color = (i == cursor) ? (Color){ 255, 255, 0, 255 } : (Color){ 200, 200, 200, 255 };
        hud_draw_text(labels[i], (float)(px + 20), (float)ry, 20, row_color);
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
            hud_draw_text(row_buf, (float)(px + 170), (float)ry, 20, row_color);
        }
    }
    hud_draw_text("D-Pad:Nav  L/R:Change  A/Enter:Select", (float)(px + 10), (float)(py + ph - 25), 14, (Color){ 150, 150, 150, 255 });
}

/* ===== Debug Visualization ===== */
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
    if (hud_font_loaded) {
        UnloadFont(hud_font);
        hud_font_loaded = 0;
    }
}
