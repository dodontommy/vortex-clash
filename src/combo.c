#include "combo.h"
#include <string.h>

/* Damage scaling table - index by hit count (0 = first hit) */
const int DAMAGE_SCALING[] = {
    100,  /* Hit 1 */
    98,   /* Hit 2 */
    96,   /* Hit 3 */
    92,   /* Hit 4 */
    88,   /* Hit 5 */
    84,   /* Hit 6 */
    80,   /* Hit 7 */
    76,   /* Hit 8 */
    72,   /* Hit 9+ */
    72,
    72,
    72,
    72,
    72,
    72,
    72
};

void combo_init(ComboState *combo) {
    memset(combo, 0, sizeof(ComboState));
}

void combo_on_hit(ComboState *combo, int base_damage, int is_light_starter) {
    combo->hit_count++;

    /* Calculate scaled damage */
    int scaling = DAMAGE_SCALING[combo->hit_count - 1];

    /* Apply light starter additional reduction */
    if (is_light_starter) {
        scaling = (scaling * LIGHT_STARTER_MULT) / 100;
    }
    
    /* Calculate actual damage */
    int scaled = (base_damage * scaling) / 100;
    combo->total_damage += scaled;
    combo->combo_damage += scaled;
    
    /* Update hitstun decay level */
    if (combo->hit_count >= 12) {
        combo->hitstun_decay_level = 3;
    } else if (combo->hit_count >= 9) {
        combo->hitstun_decay_level = 2;
    } else if (combo->hit_count >= 6) {
        combo->hitstun_decay_level = 1;
    } else {
        combo->hitstun_decay_level = 0;
    }
}

int combo_get_scaled_damage(ComboState *combo, int base_damage) {
    if (combo->hit_count == 0) {
        return base_damage;
    }
    
    int idx = combo->hit_count - 1;
    if (idx > 15) idx = 15;
    
    int scaling = DAMAGE_SCALING[idx];
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
        case 0: return 100;  /* Hits 1-5: 100% */
        case 1: return 85;   /* Hits 6-8: 85% */
        case 2: return 70;  /* Hits 9-11: 70% */
        case 3: return 55;  /* Hits 12+: 55% */
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
