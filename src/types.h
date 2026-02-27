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

/* Stage (world) dimensions — larger than screen, camera shows a portion */
#define STAGE_WIDTH 3200
#define STAGE_HEIGHT 720

/* Physics constants */
#define GRAVITY (FIXED_FROM_INT(1) / 2)  /* ~0.5 pixels/frame^2 */
#define JUMP_VELOCITY FIXED_FROM_INT(-12)
#define GROUND_Y 600  /* Y position of ground in screen coords */

/* Air movement */
#define DASH_JUMP_MAX_SPEED FIXED_FROM_INT(12)  /* Cap horizontal air speed from dash jumps (3x walk) */
#define AIR_FRICTION (FIXED_FROM_INT(1) / 16)   /* Gentle air drag on dash jumps: ~0.0625 px/frame^2 */

#endif /* TYPES_H */
