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
#define INPUT_ASSIST (1 << 7)

/* Composite inputs */
#define INPUT_THROWN (INPUT_LIGHT | INPUT_MEDIUM)  /* L+M = throw */
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
MotionType input_detect_motion(const InputBuffer *buf);

/* Get the current raw input (most recent) */
uint32_t input_get_current(const InputBuffer *buf);

/* Input source types */
typedef enum {
    INPUT_SOURCE_KEYBOARD,
    INPUT_SOURCE_GAMEPAD
} InputSource;

/* Poll input from keyboard and/or gamepad for a player */
uint32_t input_poll(int player_id, InputSource source);

/* Helper macro */
#define INPUT_HAS(in, bit) (((in) & (bit)) != 0)

#endif /* INPUT_H */