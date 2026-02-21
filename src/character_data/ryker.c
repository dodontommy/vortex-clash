#include "character.h"
#include <stddef.h>

/* Ryker's normal attacks */
static const MoveData RYKER_5L = {
    .name = "5L",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 22,
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 10,
    .damage = 1000,
    .hitstun = 12,
    .blockstun = 8,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = 0,
    .x_offset = 40,
    .y_offset = 20,
    .width = 30,
    .height = 30,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_JUMP_CANCEL,
    .meter_cost = 0,
    .meter_gain = 50
};

static const MoveData RYKER_5M = {
    .name = "5M",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 28,
    .startup_frames = 8,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 14,
    .damage = 1500,
    .hitstun = 15,
    .blockstun = 10,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(4),
    .knockback_y = 0,
    .x_offset = 50,
    .y_offset = 15,
    .width = 40,
    .height = 35,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_JUMP_CANCEL,
    .meter_cost = 0,
    .meter_gain = 75
};

static const MoveData RYKER_5H = {
    .name = "5H",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 40,
    .startup_frames = 12,
    .active_start = 0,
    .active_end = 3,
    .recovery_frames = 20,
    .damage = 2000,
    .hitstun = 20,
    .blockstun = 14,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(6),
    .knockback_y = FIXED_FROM_INT(-2),
    .x_offset = 55,
    .y_offset = 10,
    .width = 50,
    .height = 40,
    .properties = MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL | MOVE_PROP_WALL_BOUNCE,
    .meter_cost = 0,
    .meter_gain = 100
};

static const MoveData RYKER_2L = {
    .name = "2L",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 18,
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 8,
    .damage = 800,
    .hitstun = 10,
    .blockstun = 7,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(2),
    .knockback_y = 0,
    .x_offset = 30,
    .y_offset = 35,
    .width = 35,
    .height = 25,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_JUMP_CANCEL,
    .meter_cost = 0,
    .meter_gain = 40
};

static const MoveData RYKER_2M = {
    .name = "2M",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 26,
    .startup_frames = 7,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 12,
    .damage = 1200,
    .hitstun = 13,
    .blockstun = 9,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = 0,
    .x_offset = 40,
    .y_offset = 30,
    .width = 40,
    .height = 30,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_JUMP_CANCEL,
    .meter_cost = 0,
    .meter_gain = 60
};

static const MoveData RYKER_2H = {
    .name = "2H",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 50,
    .startup_frames = 10,
    .active_start = 0,
    .active_end = 3,
    .recovery_frames = 30,
    .damage = 2200,
    .hitstun = 25,
    .blockstun = 16,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(5),
    .knockback_y = FIXED_FROM_INT(-8),
    .x_offset = 45,
    .y_offset = 20,
    .width = 55,
    .height = 35,
    .properties = MOVE_PROP_LAUNCHER | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 110
};

static const MoveData RYKER_JL = {
    .name = "j.L",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 18,
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 8,
    .damage = 900,
    .hitstun = 10,
    .blockstun = 7,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(2),
    .knockback_y = FIXED_FROM_INT(1),
    .x_offset = 35,
    .y_offset = 10,
    .width = 30,
    .height = 25,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_OTG,
    .meter_cost = 0,
    .meter_gain = 45
};

static const MoveData RYKER_JM = {
    .name = "j.M",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 24,
    .startup_frames = 7,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 12,
    .damage = 1300,
    .hitstun = 14,
    .blockstun = 9,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = FIXED_FROM_INT(2),
    .x_offset = 45,
    .y_offset = 5,
    .width = 40,
    .height = 30,
    .properties = MOVE_PROP_CHAIN | MOVE_PROP_OTG,
    .meter_cost = 0,
    .meter_gain = 65
};

static const MoveData RYKER_JH = {
    .name = "j.H",
    .move_type = MOVE_TYPE_NORMAL,
    .total_frames = 36,
    .startup_frames = 10,
    .active_start = 0,
    .active_end = 4,
    .recovery_frames = 18,
    .damage = 1800,
    .hitstun = 18,
    .blockstun = 12,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(5),
    .knockback_y = FIXED_FROM_INT(4),
    .x_offset = 50,
    .y_offset = 0,
    .width = 50,
    .height = 35,
    .properties = MOVE_PROP_SLIDING_KD | MOVE_PROP_OTG,
    .meter_cost = 0,
    .meter_gain = 90
};

/* Ryker's special moves */
static const MoveData RYKER_FIREBALL_L = {
    .name = "236L",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 50,
    .startup_frames = 14,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 22,
    .damage = 1200,
    .hitstun = 16,
    .blockstun = 12,
    .chip_damage = 120,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(4),
    .knockback_y = 0,
    .x_offset = 60,
    .y_offset = 20,
    .width = 35,
    .height = 25,
    .properties = MOVE_PROP_PROJECTILE | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 60
};

static const MoveData RYKER_FIREBALL_M = {
    .name = "236M",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 55,
    .startup_frames = 16,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 24,
    .damage = 1400,
    .hitstun = 18,
    .blockstun = 14,
    .chip_damage = 140,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(5),
    .knockback_y = 0,
    .x_offset = 70,
    .y_offset = 20,
    .width = 40,
    .height = 25,
    .properties = MOVE_PROP_PROJECTILE | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 70
};

static const MoveData RYKER_FIREBALL_H = {
    .name = "236H",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 60,
    .startup_frames = 18,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 26,
    .damage = 1600,
    .hitstun = 20,
    .blockstun = 16,
    .chip_damage = 160,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(6),
    .knockback_y = 0,
    .x_offset = 80,
    .y_offset = 20,
    .width = 45,
    .height = 25,
    .properties = MOVE_PROP_PROJECTILE | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 80
};

static const MoveData RYKER_UPPERCUT_L = {
    .name = "623L",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 48,
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 7,
    .recovery_frames = 30,
    .damage = 1600,
    .hitstun = 22,
    .blockstun = 16,
    .chip_damage = 160,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = FIXED_FROM_INT(-10),
    .x_offset = 30,
    .y_offset = 0,
    .width = 35,
    .height = 60,
    .properties = MOVE_PROP_INVINCIBLE | MOVE_PROP_LAUNCHER | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 80
};

static const MoveData RYKER_UPPERCUT_M = {
    .name = "623M",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 54,
    .startup_frames = 7,
    .active_start = 0,
    .active_end = 9,
    .recovery_frames = 32,
    .damage = 1800,
    .hitstun = 24,
    .blockstun = 18,
    .chip_damage = 180,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(4),
    .knockback_y = FIXED_FROM_INT(-12),
    .x_offset = 35,
    .y_offset = 0,
    .width = 40,
    .height = 65,
    .properties = MOVE_PROP_INVINCIBLE | MOVE_PROP_LAUNCHER | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 90
};

static const MoveData RYKER_UPPERCUT_H = {
    .name = "623H",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 60,
    .startup_frames = 9,
    .active_start = 0,
    .active_end = 11,
    .recovery_frames = 34,
    .damage = 2000,
    .hitstun = 26,
    .blockstun = 20,
    .chip_damage = 200,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(5),
    .knockback_y = FIXED_FROM_INT(-14),
    .x_offset = 40,
    .y_offset = 0,
    .width = 45,
    .height = 70,
    .properties = MOVE_PROP_INVINCIBLE | MOVE_PROP_LAUNCHER | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 100
};

static const MoveData RYKER_SLIDE_L = {
    .name = "214L",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 42,
    .startup_frames = 12,
    .active_start = 0,
    .active_end = 3,
    .recovery_frames = 22,
    .damage = 1400,
    .hitstun = 18,
    .blockstun = 12,
    .chip_damage = 140,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(8),
    .knockback_y = 0,
    .x_offset = 70,
    .y_offset = 25,
    .width = 60,
    .height = 25,
    .properties = MOVE_PROP_LOW | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 70
};

static const MoveData RYKER_SLIDE_M = {
    .name = "214M",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 48,
    .startup_frames = 14,
    .active_start = 0,
    .active_end = 4,
    .recovery_frames = 24,
    .damage = 1600,
    .hitstun = 20,
    .blockstun = 14,
    .chip_damage = 160,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(10),
    .knockback_y = 0,
    .x_offset = 80,
    .y_offset = 25,
    .width = 70,
    .height = 25,
    .properties = MOVE_PROP_LOW | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 80
};

static const MoveData RYKER_SLIDE_H = {
    .name = "214H",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 54,
    .startup_frames = 16,
    .active_start = 0,
    .active_end = 5,
    .recovery_frames = 28,
    .damage = 1800,
    .hitstun = 22,
    .blockstun = 16,
    .chip_damage = 180,
    .hit_type = HIT_TYPE_LOW,
    .knockback_x = FIXED_FROM_INT(12),
    .knockback_y = 0,
    .x_offset = 90,
    .y_offset = 25,
    .width = 80,
    .height = 25,
    .properties = MOVE_PROP_LOW | MOVE_PROP_SLIDING_KD | MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 90
};

static const MoveData RYKER_DIVEAK_L = {
    .name = "j.214L",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 35,
    .startup_frames = 10,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 12,
    .damage = 1300,
    .hitstun = 16,
    .blockstun = 10,
    .chip_damage = 130,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = FIXED_FROM_INT(5),
    .x_offset = 30,
    .y_offset = 10,
    .width = 35,
    .height = 35,
    .properties = MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 65
};

static const MoveData RYKER_DIVEAK_M = {
    .name = "j.214M",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 40,
    .startup_frames = 12,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 14,
    .damage = 1500,
    .hitstun = 18,
    .blockstun = 12,
    .chip_damage = 150,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(4),
    .knockback_y = FIXED_FROM_INT(6),
    .x_offset = 35,
    .y_offset = 10,
    .width = 40,
    .height = 40,
    .properties = MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 75
};

static const MoveData RYKER_DIVEAK_H = {
    .name = "j.214H",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 45,
    .startup_frames = 14,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 16,
    .damage = 1700,
    .hitstun = 20,
    .blockstun = 14,
    .chip_damage = 170,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(5),
    .knockback_y = FIXED_FROM_INT(7),
    .x_offset = 40,
    .y_offset = 10,
    .width = 45,
    .height = 45,
    .properties = MOVE_PROP_SPECIAL_CANCEL | MOVE_PROP_SUPER_CANCEL,
    .meter_cost = 0,
    .meter_gain = 85
};

/* Ryker's supers */
static const MoveData RYKER_HADOU_BARRAGE = {
    .name = "236LM (Lvl 1)",
    .move_type = MOVE_TYPE_SUPER,
    .total_frames = 80,
    .startup_frames = 11,
    .active_start = 3,
    .active_end = 20,
    .recovery_frames = 40,
    .damage = 2500,
    .hitstun = 30,
    .blockstun = 22,
    .chip_damage = 250,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(8),
    .knockback_y = FIXED_FROM_INT(-4),
    .x_offset = 100,
    .y_offset = 15,
    .width = 80,
    .height = 40,
    .properties = MOVE_PROP_SUPER_CANCEL | MOVE_PROP_PROJECTILE,
    .meter_cost = 1000,
    .meter_gain = 0
};

static const MoveData RYKER_RUSH_COMBO = {
    .name = "214LM (Lvl 1)",
    .move_type = MOVE_TYPE_SUPER,
    .total_frames = 70,
    .startup_frames = 8,
    .active_start = 2,
    .active_end = 15,
    .recovery_frames = 40,
    .damage = 2800,
    .hitstun = 35,
    .blockstun = 25,
    .chip_damage = 280,
    .hit_type = HIT_TYPE_HIGH,
    .knockback_x = FIXED_FROM_INT(10),
    .knockback_y = FIXED_FROM_INT(-6),
    .x_offset = 50,
    .y_offset = 10,
    .width = 60,
    .height = 50,
    .properties = MOVE_PROP_HARD_KD,
    .meter_cost = 1000,
    .meter_gain = 0
};

static const MoveData RYKER_ULTIMATE_BLAST = {
    .name = "236HS (Lvl 3)",
    .move_type = MOVE_TYPE_SUPER,
    .total_frames = 120,
    .startup_frames = 5,
    .active_start = 5,
    .active_end = 25,
    .recovery_frames = 80,
    .damage = 4500,
    .hitstun = 45,
    .blockstun = 35,
    .chip_damage = 450,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(15),
    .knockback_y = FIXED_FROM_INT(-8),
    .x_offset = 150,
    .y_offset = 15,
    .width = 120,
    .height = 50,
    .properties = MOVE_PROP_INVINCIBLE | MOVE_PROP_HARD_KD | MOVE_PROP_PROJECTILE,
    .meter_cost = 3000,
    .meter_gain = 0
};

/* Ryker's throw */
static const MoveData RYKER_THROW = {
    .name = "Throw",
    .move_type = MOVE_TYPE_THROW,
    .total_frames = 35,
    .startup_frames = 5,
    .active_start = 0,
    .active_end = 2,
    .recovery_frames = 25,
    .damage = 1300,
    .hitstun = 20,
    .blockstun = 0,
    .chip_damage = 0,
    .hit_type = HIT_TYPE_UNBLOCKABLE,
    .knockback_x = FIXED_FROM_INT(8),
    .knockback_y = FIXED_FROM_INT(-5),
    .x_offset = 20,
    .y_offset = 10,
    .width = 25,
    .height = 40,
    .properties = MOVE_PROP_HARD_KD,
    .meter_cost = 0,
    .meter_gain = 65
};

/* Ryker's assist move */
static const MoveData RYKER_ASSIST = {
    .name = "Assist Fireball",
    .move_type = MOVE_TYPE_SPECIAL,
    .total_frames = 55,
    .startup_frames = 16,
    .active_start = 0,
    .active_end = 0,
    .recovery_frames = 24,
    .damage = 1000,
    .hitstun = 14,
    .blockstun = 10,
    .chip_damage = 100,
    .hit_type = HIT_TYPE_MID,
    .knockback_x = FIXED_FROM_INT(3),
    .knockback_y = 0,
    .x_offset = 60,
    .y_offset = 20,
    .width = 35,
    .height = 25,
    .properties = MOVE_PROP_PROJECTILE,
    .meter_cost = 0,
    .meter_gain = 50
};

/* Ryker character definition */
static const CharacterDef RYKER_DEF = {
    .id = CHAR_RYKER,
    .name = "Ryker",
    .walk_speed_forward = FIXED_FROM_INT(4),
    .walk_speed_backward = FIXED_FROM_INT(3),
    .jump_velocity = FIXED_FROM_INT(-18),
    .air_dash_speed = FIXED_FROM_INT(8),
    .max_hp = 10000,
    .damage_multiplier = 100,
    .normals = {
        [0] = &RYKER_5L,    /* 5L */
        [1] = &RYKER_5M,    /* 5M */
        [2] = &RYKER_5H,    /* 5H */
        [3] = &RYKER_2L,    /* 2L */
        [4] = &RYKER_2M,    /* 2M */
        [5] = &RYKER_2H,    /* 2H */
        [6] = &RYKER_JL,    /* j.L */
        [7] = &RYKER_JM,    /* j.M */
        [8] = &RYKER_JH,    /* j.H */
    },
    .specials = {
        [0] = &RYKER_FIREBALL_L,   /* 236L */
        [1] = &RYKER_FIREBALL_M,   /* 236M */
        [2] = &RYKER_FIREBALL_H,   /* 236H */
        [3] = &RYKER_UPPERCUT_L,   /* 623L */
        [4] = &RYKER_UPPERCUT_M,   /* 623M */
        [5] = &RYKER_UPPERCUT_H,   /* 623H */
        [6] = &RYKER_SLIDE_L,      /* 214L */
        [7] = &RYKER_SLIDE_M,      /* 214M */
        [8] = &RYKER_SLIDE_H,      /* 214H */
        [9] = &RYKER_DIVEAK_L,     /* j.214L */
        [10] = &RYKER_DIVEAK_M,    /* j.214M */
        [11] = &RYKER_DIVEAK_H,    /* j.214H */
    },
    .supers = {
        [0] = &RYKER_HADOU_BARRAGE,
        [1] = &RYKER_RUSH_COMBO,
        [2] = &RYKER_ULTIMATE_BLAST,
    },
    .throw_move = &RYKER_THROW,
    .assist_move = &RYKER_ASSIST
};

const CharacterDef *character_get_def(CharacterId id) {
    switch (id) {
        case CHAR_RYKER:
            return &RYKER_DEF;
        default:
            return NULL;
    }
}

const MoveData *character_get_normal(CharacterId id, int input) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;
    
    /* Map input to normal index */
    int idx = -1;
    switch (input) {
        case 0: idx = 0; break;  /* 5L */
        case 1: idx = 1; break;  /* 5M */
        case 2: idx = 2; break;  /* 5H */
        case 3: idx = 3; break;  /* 2L */
        case 4: idx = 4; break;  /* 2M */
        case 5: idx = 5; break;  /* 2H */
        case 6: idx = 6; break;  /* j.L */
        case 7: idx = 7; break;  /* j.M */
        case 8: idx = 8; break;  /* j.H */
    }
    
    if (idx >= 0 && idx < 16) {
        return c->normals[idx];
    }
    return NULL;
}

const MoveData *character_get_special(CharacterId id, int motion, int button) {
    (void)id;
    (void)motion;
    (void)button;
    /* TODO: Implement motion + button lookup */
    return NULL;
}

const MoveData *character_get_super(CharacterId id, int level) {
    const CharacterDef *c = character_get_def(id);
    if (!c || level < 1 || level > 3) return NULL;
    
    /* Level 1 has 2 supers, level 3 has 1 */
    if (level == 1) {
        /* Return first L1 super (could add selector) */
        return c->supers[0];
    } else if (level == 3) {
        return c->supers[2];
    }
    return NULL;
}
