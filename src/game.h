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
    int remap_cursor;         /* 0-3: L, M, H, S */
    bool_t remap_listening;   /* TRUE when waiting for key/button press */
    int remap_target;         /* Which player's bindings to edit (0 = P1) */
} TrainingState;

/* Camera zoom limits */
#define CAMERA_ZOOM_MIN   1.5f
#define CAMERA_ZOOM_MAX   2.2f
#define CAMERA_PADDING    300  /* world pixels of margin around both players */

/* Game state with two players.
 *
 * === GGPO Serialization Notes ===
 * SIMULATION STATE (must be serialized for rollback):
 *   players[2]      — full PlayerState including CharacterState, combo, meter
 *   inputs[2]       — input buffers for motion detection
 *   hitstop_frames   — shared hitstop counter
 *   frame_count      — global frame counter
 *
 * RENDER-ONLY STATE (local, not serialized):
 *   sprites, camera, shake_*, running, debug_draw
 */
typedef struct {
    /* --- Simulation state (serialize for GGPO) --- */
    PlayerState players[2];
    InputBuffer inputs[2];
    ProjectileState projectiles;
    int hitstop_frames;
    int frame_count;

    /* --- Render-only state (local) --- */
    InputBindings bindings[2];  /* key/gamepad bindings for P1 and P2 */
    SpriteSet sprites[2];  /* sprite sets for P1 and P2 */
    Camera2D camera;
    bool_t running;
    bool_t debug_draw;
    /* Screen shake */
    float shake_amplitude;
    int shake_frames;
    int shake_frames_max;
    /* Combo counter + training mode */
    ComboDisplayState combo_display[2];
    TrainingState training;
} GameState;

/* Game functions */
void game_init(GameState *game);
void game_update(GameState *game);
void game_render(const GameState *game);
void game_shutdown(GameState *game);

#endif /* GAME_H */
