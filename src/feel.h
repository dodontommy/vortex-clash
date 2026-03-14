#ifndef FEEL_H
#define FEEL_H

/* --- Hitstop base values (frames) --- */
#define HITSTOP_NORMAL_L         4
#define HITSTOP_NORMAL_M         6
#define HITSTOP_NORMAL_H         8
#define HITSTOP_SPECIAL_L        5
#define HITSTOP_SPECIAL_M        7
#define HITSTOP_SPECIAL_H       10
#define HITSTOP_SUPER           10
#define HITSTOP_THROW           10

/* Hitstop modifiers */
#define HITSTOP_LAUNCHER_BONUS   2
#define HITSTOP_WALLBOUNCE_BONUS 2
#define HITSTOP_COUNTER_BONUS    3
#define HITSTOP_BLOCK_REDUCE_HI  3   /* subtract when base >= 8 */
#define HITSTOP_BLOCK_REDUCE_LO  2   /* subtract when base < 8 */
#define HITSTOP_BLOCK_FLOOR      2
#define HITSTOP_CAP             16

/* --- Counter-hit multipliers (N/100) --- */
#define CH_DAMAGE_MULT         130   /* 1.3x */
#define CH_HITSTUN_MULT        130   /* 1.3x */

/* --- Hit flash --- */
#define HIT_FLASH_FRAMES         4

/* --- Blue health --- */
#define BLUE_HEALTH_PERCENT     50   /* % of damage -> recoverable */
#define BLUE_HEALTH_RECOVERY     5   /* HP/frame while off-screen */

/* --- Knockdown / Wakeup --- */
#define KNOCKDOWN_FRAMES        55
#define WAKEUP_FRAMES            8

/* --- Pushblock --- */
#define PUSHBLOCK_PUSH_PX      180
#define PUSHBLOCK_LOCKOUT       24
#define PUSHBLOCK_EXTRA_BLOCKSTUN 4

/* --- Damage drain visual --- */
#define DAMAGE_DRAIN_DELAY      30

/* --- Screen shake (render-only, int*10 to avoid floats in header) --- */
#define SHAKE_MED_AMP10         15   /* 1.5f */
#define SHAKE_MED_FRAMES         3
#define SHAKE_HVY_AMP10         25   /* 2.5f */
#define SHAKE_HVY_FRAMES         4
#define SHAKE_CIN_AMP10         35   /* 3.5f */
#define SHAKE_CIN_FRAMES         5

/* --- Impact pop (grounded hit visual lift, pixels) --- */
#define IMPACT_POP_L             1
#define IMPACT_POP_M             2
#define IMPACT_POP_H             3

/* --- Juggle / Air combo --- */
#define JUGGLE_GRAVITY_PER_HIT  (FIXED_FROM_INT(1) / 4)  /* +0.25 downward per air hit */
#define JUGGLE_HITSTUN_FLOOR    8   /* minimum air hitstun frames */
#define JUGGLE_MIN_VY           FIXED_FROM_INT(-6)  /* minimum upward bounce per air hit */

/* --- Damage scaling --- */
#define DAMAGE_SCALING_FLOOR    20
#define LIGHT_STARTER_MULT      80

/* --- Super flash (simulation-pausing freeze) --- */
#define SUPER_FLASH_LVL1        30   /* frames */
#define SUPER_FLASH_LVL3        45
#define SUPER_FLASH_DARKEN     160   /* alpha 0-255, render-only */

#endif /* FEEL_H */
