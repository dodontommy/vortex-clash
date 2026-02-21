#include "character.h"

/* Include all character data */
#include "character_data/ryker.c"

/* Get character definition - defined in ryker.c */
extern const CharacterDef *character_get_def(CharacterId id);

/* Character IDs enum is in character.h */

const CharacterDef *character_get_by_id(CharacterId id) {
    return character_get_def(id);
}
