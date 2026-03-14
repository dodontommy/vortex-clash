#include "combo.h"
#include <string.h>

/* Damage scaling table - index by hit count (0 = first hit) */
const int DAMAGE_SCALING[] = {
    100,  /* Hit 1 */
    96,   /* Hit 2 */
    90,   /* Hit 3 */
    82,   /* Hit 4 */
    72,   /* Hit 5 */
    62,   /* Hit 6 */
    52,   /* Hit 7 */
    42,   /* Hit 8 */
    34,   /* Hit 9 */
    28,   /* Hit 10 */
    24,   /* Hit 11 */
    20,   /* Hit 12+ */
    20,
    20,
    20,
    20
};

void combo_init(ComboState *combo) {
    memset(combo, 0, sizeof(ComboState));
}

int combo_apply_hit(ComboState *combo, int base_damage, int is_light_hit) {
    int idx = combo->hit_count;
    if (idx > 15) idx = 15;

    if (combo->hit_count == 0 && is_light_hit) {
        combo->light_starter = 1;
    }

    /* Calculate scaled damage from current hit index (0=first hit). */
    int scaling = DAMAGE_SCALING[idx];

    /* Light-starter reduction applies to the whole combo. */
    if (combo->light_starter) {
        scaling = (scaling * LIGHT_STARTER_MULT) / 100;
    }

    int scaled = (base_damage * scaling) / 100;
    combo->hit_count++;
    combo->total_damage += scaled;
    combo->combo_damage += scaled;

    /* Update hitstun decay level */
    if (combo->hit_count >= 13) {
        combo->hitstun_decay_level = 4;
    } else if (combo->hit_count >= 10) {
        combo->hitstun_decay_level = 3;
    } else if (combo->hit_count >= 7) {
        combo->hitstun_decay_level = 2;
    } else if (combo->hit_count >= 4) {
        combo->hitstun_decay_level = 1;
    } else {
        combo->hitstun_decay_level = 0;
    }

    return scaled;
}

void combo_on_hit(ComboState *combo, int base_damage, int is_light_starter) {
    (void)combo_apply_hit(combo, base_damage, is_light_starter);
}

int combo_get_scaled_damage(ComboState *combo, int base_damage) {
    int idx = combo->hit_count;
    if (idx > 15) idx = 15;

    int scaling = DAMAGE_SCALING[idx];
    if (combo->light_starter) {
        scaling = (scaling * LIGHT_STARTER_MULT) / 100;
    }
    return (base_damage * scaling) / 100;
}

void combo_on_block(ComboState *combo) {
    combo_reset(combo);
}

void combo_reset(ComboState *combo) {
    memset(combo, 0, sizeof(ComboState));
}

/* Hitstun decay - returns percentage of normal hitstun */
int combo_get_hitstun_decay(ComboState *combo) {
    switch (combo->hitstun_decay_level) {
        case 0: return 100;  /* Hits 1-3: 100% */
        case 1: return 85;   /* Hits 4-6: 85% */
        case 2: return 70;   /* Hits 7-9: 70% */
        case 3: return 55;   /* Hits 10-12: 55% */
        case 4: return 40;   /* Hits 13+: 40% */
        default: return 100;
    }
}

int combo_can_otg(ComboState *combo) {
    return combo->otg_used == 0;
}

int combo_can_wall_bounce(ComboState *combo) {
    return combo->wall_bounce_used == 0;
}

int combo_can_ground_bounce(ComboState *combo) {
    return combo->ground_bounce_used == 0;
}

void combo_use_otg(ComboState *combo) {
    combo->otg_used = 1;
}

void combo_use_wall_bounce(ComboState *combo) {
    combo->wall_bounce_used = 1;
}

void combo_use_ground_bounce(ComboState *combo) {
    combo->ground_bounce_used = 1;
}
