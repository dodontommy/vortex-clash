/*
 * Character system — adding a new character:
 * 1. Create src/character_data/<name>.c with all MoveData + CharacterDef
 * 2. Add a CHAR_<NAME> entry to CharacterId enum in character.h
 * 3. #include the data file below
 * 4. Add a case to the switch in character_get_def()
 */

#include "character.h"

/* Include all character data (data-only, no functions) */
#include "character_data/ryker.c"
/* #include "character_data/zara.c" */

const CharacterDef *character_get_def(CharacterId id) {
    switch (id) {
        case CHAR_RYKER: return &RYKER_DEF;
        default: return NULL;
    }
}

const MoveData *character_get_normal(CharacterId id, int input) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;
    if (input >= 0 && input < 16) return c->normals[input];
    return NULL;
}

const MoveData *character_get_special(CharacterId id, int motion, int button) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;
    int strength = (button & INPUT_LIGHT) ? 0 : (button & INPUT_MEDIUM) ? 1 : 2;
    switch (motion) {
        case MOTION_QCF: return c->specials[0 + strength];   /* 236L/M/H */
        case MOTION_DP:  return c->specials[3 + strength];   /* 623L/M/H */
        case MOTION_QCB: return c->specials[6 + strength];   /* 214L/M/H */
        default: return NULL;
    }
}

const MoveData *character_get_super(CharacterId id, int level) {
    const CharacterDef *c = character_get_def(id);
    if (!c || level < 1 || level > 3) return NULL;

    /* Level 1 has 2 supers, level 3 has 1 */
    if (level == 1) {
        return c->supers[0];
    } else if (level == 3) {
        return c->supers[2];
    }
    return NULL;
}

const MoveData *character_get_throw(CharacterId id) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;
    return c->throw_move;
}

const MoveData *character_get_assist_move(CharacterId id) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;
    return c->assist_move;
}

const MoveData *character_get_move_by_slot(CharacterId id, int attack_ref_type, int index) {
    const CharacterDef *c = character_get_def(id);
    if (!c) return NULL;

    switch (attack_ref_type) {
        case ATTACK_REF_NORMAL:
            if (index >= 0 && index < 16) return c->normals[index];
            return NULL;
        case ATTACK_REF_SPECIAL:
            if (index >= 0 && index < 16) return c->specials[index];
            return NULL;
        case ATTACK_REF_SUPER:
            if (index >= 0 && index < 4) return c->supers[index];
            return NULL;
        case ATTACK_REF_THROW:
            return c->throw_move;
        case ATTACK_REF_ASSIST:
            return c->assist_move;
        default:
            return NULL;
    }
}
