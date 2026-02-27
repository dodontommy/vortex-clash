#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/* Input button bits (matching player.h) */
#define INPUT_UP     (1 << 0)
#define INPUT_DOWN   (1 << 1)
#define INPUT_LEFT   (1 << 2)
#define INPUT_RIGHT  (1 << 3)
#define INPUT_LIGHT  (1 << 4)
#define INPUT_MEDIUM (1 << 5)
#define INPUT_HEAVY  (1 << 6)
#define INPUT_SPECIAL (1 << 7)
#define INPUT_A1      (1 << 8)

/* Composite inputs */
#define INPUT_SUPER  (INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY)  /* L+M+H = super */

/* Motion types */
typedef enum {
    MOTION_NONE,
    MOTION_QCF,       /* 236 - quarter circle forward */
    MOTION_QCB,       /* 214 - quarter circle back */
    MOTION_DP,        /* 623 - dragon punch */
    MOTION_RDP,       /* 63214 - reverse dragon punch */
    MOTION_360,       /* full rotation */
    MOTION_HCF,       /* 41236 */
    MOTION_HCB,       /* 63214 */
    MOTION_DOUBLE_TAP_FORWARD,  /* 66 */
    MOTION_DOUBLE_TAP_BACK      /* 44 */
} MotionType;

/* Input buffer - ring buffer for input history */
#define INPUT_BUFFER_SIZE 60
typedef struct {
    uint32_t buffer[INPUT_BUFFER_SIZE];
    int head;           /* Most recent input index */
    int count;          /* Number of valid inputs */
} InputBuffer;

/* Initialize input buffer */
void input_init(InputBuffer *buf);

/* Update buffer with new raw input */
void input_update(InputBuffer *buf, uint32_t raw);

/* Check if button was pressed this frame (rising edge) */
int input_button_pressed(const InputBuffer *buf, uint32_t button);

/* Check if button is held this frame */
int input_button_held(const InputBuffer *buf, uint32_t button);

/* Detect motion in input history */
MotionType input_detect_motion(const InputBuffer *buf, int facing);

/* Get the current raw input (most recent) */
uint32_t input_get_current(const InputBuffer *buf);

/* Input source types */
typedef enum {
    INPUT_SOURCE_KEYBOARD,
    INPUT_SOURCE_GAMEPAD
} InputSource;

/* Poll input from keyboard and/or gamepad for a player */
uint32_t input_poll(int player_id, InputSource source);

/* Key/gamepad remapping */
#define REMAP_BUTTON_COUNT 5  /* L, M, H, S, A1 */
typedef struct {
    int keyboard_key;    /* Raylib KEY_* constant */
    int gamepad_button;  /* Raylib GAMEPAD_BUTTON_* constant, -1 = unbound */
} ButtonBinding;

typedef struct {
    ButtonBinding buttons[REMAP_BUTTON_COUNT]; /* 0=L, 1=M, 2=H, 3=S */
} InputBindings;

/* Initialize bindings to default keys for player_id (1 or 2) */
void input_bindings_init(InputBindings *b, int player_id);

/* Poll input using custom bindings (directions hardcoded, buttons from bindings) */
uint32_t input_poll_bound(int player_id, InputSource source, const InputBindings *b);

/* Human-readable key/button names for the remap menu */
const char *input_key_name(int key);
const char *input_gamepad_button_name(int button);

/* Scan for newly pressed key/gamepad button (for remapping). Returns -1 if none. */
int input_scan_key(void);
int input_scan_gamepad(int gamepad);

/* Helper macro */
#define INPUT_HAS(in, bit) (((in) & (bit)) != 0)

#endif /* INPUT_H */