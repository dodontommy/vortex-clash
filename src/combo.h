#ifndef COMBO_H
#define COMBO_H

#include "types.h"

/* Combo state for tracking hits and scaling */
typedef struct {
    int hit_count;           /* Total hits in current combo */
    int total_damage;        /* Total damage dealt in combo */
    int combo_damage;        /* Combo damage for this opponent */
    int otg_used;            /* OTG used this combo */
    int wall_bounce_used;    /* Wall bounce used this combo */
    int ground_bounce_used;  /* Ground bounce used this combo */
    int hitstun_decay_level; /* Hitstun decay tier (0-3) */
} ComboState;

/* Damage scaling table (indexed by hit count, 0-indexed) */
extern const int DAMAGE_SCALING[];
#define MIN_SCALING 72
#define LIGHT_STARTER_MULT 80

/* Combo functions */
void combo_init(ComboState *combo);
void combo_on_hit(ComboState *combo, int base_damage, int is_light_starter);
int combo_get_scaled_damage(ComboState *combo, int base_damage);
void combo_on_block(ComboState *combo);
void combo_reset(ComboState *combo);

/* Hitstun decay - returns multiplier for hitstun frames */
int combo_get_hitstun_decay(ComboState *combo);

/* Check if special combo enders are available */
int combo_can_otg(ComboState *combo);
int combo_can_wall_bounce(ComboState *combo);
int combo_can_ground_bounce(ComboState *combo);

/* Mark combo enders as used */
void combo_use_otg(ComboState *combo);
void combo_use_wall_bounce(ComboState *combo);
void combo_use_ground_bounce(ComboState *combo);

#endif /* COMBO_H */
