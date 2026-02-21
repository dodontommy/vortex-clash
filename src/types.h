#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Fixed-point type: 16.16 signed */
typedef int32_t fixed_t;
#define FIXED_SHIFT 16
#define FIXED_ONE (1 << FIXED_SHIFT)

/* Boolean */
#define TRUE 1
#define FALSE 0
typedef int32_t bool_t;

/* Math helpers */
#define FIXED_MUL(a, b) ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define FIXED_DIV(a, b) ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))
#define FIXED_FROM_INT(x) ((fixed_t)((x) << FIXED_SHIFT))
#define FIXED_TO_INT(x) ((x) >> FIXED_SHIFT)

/* Screen dimensions */
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

/* Game states */
typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_PAUSED,
    STATE_GAMEOVER
} game_state_t;

/* Player directions */
typedef enum {
    DIR_LEFT = -1,
    DIR_RIGHT = 1
} direction_t;

/* Hitbox simple rect */
typedef struct {
    int x, y;
    int w, h;
} rect_t;

#endif /* TYPES_H */
