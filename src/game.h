#ifndef GAME_H
#define GAME_H

#include "types.h"
#include "player.h"
#include "projectile.h"
#include "sprite.h"
#include "input.h"
#include <raylib.h>

/* Combo counter display (render-only, not serialized) */
typedef struct {
    int display_hit_count;
    int display_damage;
    int fade_timer;           /* 60 = 1s at 60fps */
} ComboDisplayState;

#define MAX_HIT_SPARKS 24
typedef struct {
    int active;
    float x, y;
    float vx, vy;
    int life;
    int life_max;
    unsigned char r, g, b, a;
} HitSpark;

/* Training mode enums */
typedef enum {
    BLOCK_NONE, BLOCK_ALL, BLOCK_AFTER_FIRST,
    BLOCK_CROUCH, BLOCK_STAND, BLOCK_MODE_COUNT
} DummyBlockMode;

typedef enum {
    DUMMY_STAND, DUMMY_CROUCH, DUMMY_JUMP, DUMMY_STATE_COUNT
} DummyStateMode;

typedef struct {
    bool_t active;
    bool_t menu_open;
    int cursor;               /* 0-5: Block, State, Counter Hit, HP Reset, Controls, Exit Game */
    DummyBlockMode block_mode;
    DummyStateMode dummy_state;
    bool_t counter_hit;
    bool_t hp_meter_reset;
    /* Key remap sub-menu */
    bool_t remap_open;        /* TRUE when remap sub-menu is visible */
    int remap_cursor;         /* 0-4: L, M, H, S, A1 */
    bool_t remap_listening;   /* TRUE when waiting for key/button press */
    int remap_target;         /* Which player's bindings to edit (0 = P1) */
} TrainingState;

/* Camera zoom limits */
#define CAMERA_ZOOM_MIN   1.5f
#define CAMERA_ZOOM_MAX   2.2f
#define CAMERA_PADDING    300  /* world pixels of margin around both players */

/* Simulation state — flat, deterministic, serializable for GGPO rollback.
 * No pointers.
 * No floats. All values are int or fixed_t. */
typedef struct {
    PlayerState players[2];
    InputBuffer inputs[2];
    ProjectileState projectiles;
    int hitstop_frames;
    int frame_count;
    int ko_winner;          /* -1 = none, 0 = P1 wins, 1 = P2 wins */
    int ko_timer;           /* Freeze countdown after KO */
    int timer_frames;       /* Countdown from 5940 (99s * 60fps), 0 = time up */
    TrainingState training; /* Affects simulation: block overrides, counter-hit, HP reset */
} GameState;

/* Render-only state — local to each client, NOT serialized for rollback.
 * Contains floats, GPU resources, and visual-only data. */
typedef struct {
    InputBindings bindings[2];
    SpriteSet sprites[2];
    Camera2D camera;
    bool_t running;
    bool_t debug_draw;
    /* Screen shake */
    float shake_amplitude;
    int shake_frames;
    int shake_frames_max;
    float shake_dir_x;
    float shake_dir_y;
    /* Hit spark ring-buffer */
    HitSpark hit_sparks[MAX_HIT_SPARKS];
    /* Combo counter display */
    ComboDisplayState combo_display[2];
    /* Damage drain effect */
    float damage_display_hp[2][2];  /* [player][char_slot] lerp target */
    int damage_drain_delay[2][2];   /* 30f delay before drain starts */
} GameRenderState;

#define KO_FREEZE_FRAMES 120
#define TIMER_START_FRAMES 5940  /* 99 seconds * 60 fps */

/* Game functions */
void game_init(GameState *game, GameRenderState *render);
void game_update(GameState *game, GameRenderState *render);
void game_render(const GameState *game, const GameRenderState *render);
void game_shutdown(GameState *game, GameRenderState *render);

#endif /* GAME_H */
