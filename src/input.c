#include "input.h"
#ifndef TESTING_HEADLESS
#include <raylib.h>
#endif
#include <string.h>

/* Motion detection constants */
#define MOTION_QCF_FRAMES   12
#define MOTION_DP_FRAMES    15
#define MOTION_360_FRAMES   20

/* Direction values for motion detection */
#define DIR_NONE        0
#define DIR_UP          1
#define DIR_DOWN        2
#define DIR_LEFT        3
#define DIR_RIGHT       4
#define DIR_UP_RIGHT    5
#define DIR_UP_LEFT     6
#define DIR_DOWN_RIGHT  7
#define DIR_DOWN_LEFT   8

static int get_direction(uint32_t input) {
    int dir = DIR_NONE;
    if (INPUT_HAS(input, INPUT_DOWN)) {
        if (INPUT_HAS(input, INPUT_RIGHT)) dir = DIR_DOWN_RIGHT;
        else if (INPUT_HAS(input, INPUT_LEFT)) dir = DIR_DOWN_LEFT;
        else dir = DIR_DOWN;
    } else if (INPUT_HAS(input, INPUT_UP)) {
        if (INPUT_HAS(input, INPUT_RIGHT)) dir = DIR_UP_RIGHT;
        else if (INPUT_HAS(input, INPUT_LEFT)) dir = DIR_UP_LEFT;
        else dir = DIR_UP;
    } else if (INPUT_HAS(input, INPUT_RIGHT)) {
        dir = DIR_RIGHT;
    } else if (INPUT_HAS(input, INPUT_LEFT)) {
        dir = DIR_LEFT;
    }
    return dir;
}

static int to_cardinal(int dir) {
    switch (dir) {
        case DIR_UP: case DIR_UP_RIGHT: case DIR_UP_LEFT: return DIR_UP;
        case DIR_DOWN: case DIR_DOWN_RIGHT: case DIR_DOWN_LEFT: return DIR_DOWN;
        case DIR_LEFT: return DIR_LEFT;
        case DIR_RIGHT: return DIR_RIGHT;
        default: return DIR_NONE;
    }
}

void input_init(InputBuffer *buf) {
    memset(buf, 0, sizeof(InputBuffer));
    buf->head = -1;
    buf->count = 0;
}

void input_update(InputBuffer *buf, uint32_t raw) {
    buf->head = (buf->head + 1) % INPUT_BUFFER_SIZE;
    buf->buffer[buf->head] = raw;
    if (buf->count < INPUT_BUFFER_SIZE) buf->count++;
}

uint32_t input_get_current(const InputBuffer *buf) {
    if (buf->count == 0) return 0;
    return buf->buffer[buf->head];
}

int input_button_pressed(const InputBuffer *buf, uint32_t button) {
    if (buf->count < 2) {
        return INPUT_HAS(input_get_current(buf), button) ? 1 : 0;
    }
    int prev = (buf->head - 1 + INPUT_BUFFER_SIZE) % INPUT_BUFFER_SIZE;
    uint32_t current = buf->buffer[buf->head];
    uint32_t previous = buf->buffer[prev];
    return (INPUT_HAS(current, button) && !INPUT_HAS(previous, button)) ? 1 : 0;
}

int input_button_held(const InputBuffer *buf, uint32_t button) {
    return INPUT_HAS(input_get_current(buf), button) ? 1 : 0;
}

static uint32_t get_input_at(const InputBuffer *buf, int frames_ago) {
    if (frames_ago >= buf->count) return 0;
    return buf->buffer[(buf->head - frames_ago + INPUT_BUFFER_SIZE) % INPUT_BUFFER_SIZE];
}

static int get_direction_at(const InputBuffer *buf, int frames_ago) {
    return get_direction(get_input_at(buf, frames_ago));
}

/* Convert raw input direction to facing-relative direction.
 * When facing right (1): left=back, right=forward (no change).
 * When facing left (-1): left=forward, right=back (mirror L/R). */
static int get_facing_direction(uint32_t input, int facing) {
    int has_up = INPUT_HAS(input, INPUT_UP);
    int has_down = INPUT_HAS(input, INPUT_DOWN);
    int has_fwd = (facing == 1) ? INPUT_HAS(input, INPUT_RIGHT) : INPUT_HAS(input, INPUT_LEFT);
    int has_back = (facing == 1) ? INPUT_HAS(input, INPUT_LEFT) : INPUT_HAS(input, INPUT_RIGHT);

    if (has_down) {
        if (has_fwd) return DIR_DOWN_RIGHT;   /* down-forward */
        if (has_back) return DIR_DOWN_LEFT;    /* down-back */
        return DIR_DOWN;
    }
    if (has_up) {
        if (has_fwd) return DIR_UP_RIGHT;
        if (has_back) return DIR_UP_LEFT;
        return DIR_UP;
    }
    if (has_fwd) return DIR_RIGHT;   /* forward */
    if (has_back) return DIR_LEFT;   /* back */
    return DIR_NONE;
}

static int get_facing_direction_at(const InputBuffer *buf, int frames_ago, int facing) {
    return get_facing_direction(get_input_at(buf, frames_ago), facing);
}

/* Scan input history for a directional sequence (oldest-to-newest).
 * seq[] is in chronological order: seq[0] is the first direction pressed.
 * We search backwards through the buffer, matching seq entries from last to
 * first so that seq[len-1] is closest to the current frame. Intermediate
 * frames that don't match any expected direction are skipped (leniency). */
static int match_motion_sequence(const InputBuffer *buf, const int *seq, int len, int max_frames, int facing) {
    if (buf->count < len) return 0;
    int limit = max_frames < buf->count ? max_frames : buf->count;
    int need = len - 1;  /* index into seq we're looking for (countdown) */
    for (int i = 0; i < limit; i++) {
        int dir = get_facing_direction_at(buf, i, facing);
        int want = seq[need];
        if (dir == want
            || (want == DIR_DOWN && (dir == DIR_DOWN_RIGHT || dir == DIR_DOWN_LEFT))
            || (want == DIR_RIGHT && (dir == DIR_DOWN_RIGHT || dir == DIR_UP_RIGHT))
            || (want == DIR_LEFT && (dir == DIR_DOWN_LEFT || dir == DIR_UP_LEFT))
            || (want == DIR_UP && (dir == DIR_UP_RIGHT || dir == DIR_UP_LEFT))) {
            need--;
            if (need < 0) return 1;
        }
    }
    return 0;
}

MotionType input_detect_motion(const InputBuffer *buf, int facing) {
    if (buf->count < 2) return MOTION_NONE;

    /* QCF: 2,3,6 or shortcuts (down, down-forward, forward) */
    {
        int qcf[] = {DIR_DOWN, DIR_DOWN_RIGHT, DIR_RIGHT};
        int qcf_s[] = {DIR_DOWN_RIGHT, DIR_RIGHT};
        if (match_motion_sequence(buf, qcf, 3, MOTION_QCF_FRAMES, facing) ||
            match_motion_sequence(buf, qcf_s, 2, MOTION_QCF_FRAMES, facing)) {
            return MOTION_QCF;
        }
    }

    /* QCB: 2,1,4 (down, down-back, back) */
    {
        int qcb[] = {DIR_DOWN, DIR_DOWN_LEFT, DIR_LEFT};
        int qcb_s[] = {DIR_DOWN_LEFT, DIR_LEFT};
        if (match_motion_sequence(buf, qcb, 3, MOTION_QCF_FRAMES, facing) ||
            match_motion_sequence(buf, qcb_s, 2, MOTION_QCF_FRAMES, facing)) {
            return MOTION_QCB;
        }
    }

    /* DP: 6,2,3 (forward, down, down-forward) */
    {
        int dp[] = {DIR_RIGHT, DIR_DOWN, DIR_DOWN_RIGHT};
        int dp_l[] = {DIR_RIGHT, DIR_DOWN_RIGHT, DIR_RIGHT};
        if (match_motion_sequence(buf, dp, 3, MOTION_DP_FRAMES, facing) ||
            match_motion_sequence(buf, dp_l, 3, MOTION_DP_FRAMES, facing)) {
            return MOTION_DP;
        }
    }

    /* 360 */
    {
        int seen[5] = {0}, n = 0;
        for (int i = 0; i < MOTION_360_FRAMES && i < buf->count; i++) {
            int c = to_cardinal(get_facing_direction_at(buf, i, facing));
            if (c != DIR_NONE && !seen[c]) { seen[c] = 1; n++; }
        }
        if (n >= 4) return MOTION_360;
    }

    return MOTION_NONE;
}

#ifndef TESTING_HEADLESS
/* Keyboard mappings */
typedef struct { int key; uint32_t input; } KeyMapping;

static const KeyMapping key_map_p1[] = {
    {KEY_W, INPUT_UP}, {KEY_S, INPUT_DOWN}, {KEY_A, INPUT_LEFT}, {KEY_D, INPUT_RIGHT},
    {KEY_V, INPUT_LIGHT}, {KEY_B, INPUT_MEDIUM}, {KEY_N, INPUT_HEAVY}, {KEY_G, INPUT_SPECIAL},
};
static const KeyMapping key_map_p2[] = {
    {KEY_UP, INPUT_UP}, {KEY_DOWN, INPUT_DOWN}, {KEY_LEFT, INPUT_LEFT}, {KEY_RIGHT, INPUT_RIGHT},
    {KEY_KP_1, INPUT_LIGHT}, {KEY_KP_2, INPUT_MEDIUM}, {KEY_KP_3, INPUT_HEAVY}, {KEY_KP_0, INPUT_SPECIAL},
};

uint32_t input_poll(int player_id, InputSource source) {
    uint32_t input = 0;
    
    if (source == INPUT_SOURCE_KEYBOARD) {
        const KeyMapping *map;
        int map_size;
        if (player_id == 1) { map = key_map_p1; map_size = 8; }
        else { map = key_map_p2; map_size = 8; }
        
        for (int i = 0; i < map_size; i++) {
            if (IsKeyDown(map[i].key)) input |= map[i].input;
        }
    }
    
    if (source == INPUT_SOURCE_GAMEPAD) {
        int gamepad = player_id - 1;
        if (IsGamepadAvailable(gamepad)) {
            /* Left stick analog */
            float ax = GetGamepadAxisMovement(gamepad, 0);
            float ay = GetGamepadAxisMovement(gamepad, 1);
            if (ay < -0.5f) input |= INPUT_UP;
            if (ay > 0.5f) input |= INPUT_DOWN;
            if (ax < -0.5f) input |= INPUT_LEFT;
            if (ax > 0.5f) input |= INPUT_RIGHT;
            /* D-pad */
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP)) input |= INPUT_UP;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) input |= INPUT_DOWN;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) input |= INPUT_LEFT;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) input |= INPUT_RIGHT;
            /* Face buttons (Xbox): X=Light, Y=Medium, B=Heavy, A=Assist */
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) input |= INPUT_LIGHT;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_UP)) input |= INPUT_MEDIUM;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) input |= INPUT_HEAVY;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) input |= INPUT_SPECIAL;
            /* Bumpers: RB=Heavy (alt), LB=Assist (alt) */
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) input |= INPUT_HEAVY;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) input |= INPUT_SPECIAL;
        }
    }
    
    return input;
}

/* Default button bindings per player */
void input_bindings_init(InputBindings *b, int player_id) {
    if (player_id == 1) {
        b->buttons[0] = (ButtonBinding){ KEY_V, GAMEPAD_BUTTON_RIGHT_FACE_LEFT };   /* L = V / X */
        b->buttons[1] = (ButtonBinding){ KEY_B, GAMEPAD_BUTTON_RIGHT_FACE_UP };     /* M = B / Y */
        b->buttons[2] = (ButtonBinding){ KEY_N, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT };  /* H = N / B */
        b->buttons[3] = (ButtonBinding){ KEY_G, GAMEPAD_BUTTON_RIGHT_FACE_DOWN };   /* S = G / A */
    } else {
        b->buttons[0] = (ButtonBinding){ KEY_KP_1, GAMEPAD_BUTTON_RIGHT_FACE_LEFT };
        b->buttons[1] = (ButtonBinding){ KEY_KP_2, GAMEPAD_BUTTON_RIGHT_FACE_UP };
        b->buttons[2] = (ButtonBinding){ KEY_KP_3, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT };
        b->buttons[3] = (ButtonBinding){ KEY_KP_0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN };
    }
}

/* Input bits corresponding to each binding index */
static const uint32_t binding_bits[REMAP_BUTTON_COUNT] = {
    INPUT_LIGHT, INPUT_MEDIUM, INPUT_HEAVY, INPUT_SPECIAL
};

uint32_t input_poll_bound(int player_id, InputSource source, const InputBindings *b) {
    uint32_t input = 0;

    if (source == INPUT_SOURCE_KEYBOARD) {
        /* Directions are hardcoded */
        if (player_id == 1) {
            if (IsKeyDown(KEY_W)) input |= INPUT_UP;
            if (IsKeyDown(KEY_S)) input |= INPUT_DOWN;
            if (IsKeyDown(KEY_A)) input |= INPUT_LEFT;
            if (IsKeyDown(KEY_D)) input |= INPUT_RIGHT;
        } else {
            if (IsKeyDown(KEY_UP))    input |= INPUT_UP;
            if (IsKeyDown(KEY_DOWN))  input |= INPUT_DOWN;
            if (IsKeyDown(KEY_LEFT))  input |= INPUT_LEFT;
            if (IsKeyDown(KEY_RIGHT)) input |= INPUT_RIGHT;
        }
        /* Action buttons from bindings */
        for (int i = 0; i < REMAP_BUTTON_COUNT; i++) {
            if (b->buttons[i].keyboard_key >= 0 && IsKeyDown(b->buttons[i].keyboard_key))
                input |= binding_bits[i];
        }
    }

    if (source == INPUT_SOURCE_GAMEPAD) {
        int gamepad = player_id - 1;
        if (IsGamepadAvailable(gamepad)) {
            /* Analog stick */
            float ax = GetGamepadAxisMovement(gamepad, 0);
            float ay = GetGamepadAxisMovement(gamepad, 1);
            if (ay < -0.5f) input |= INPUT_UP;
            if (ay > 0.5f)  input |= INPUT_DOWN;
            if (ax < -0.5f) input |= INPUT_LEFT;
            if (ax > 0.5f)  input |= INPUT_RIGHT;
            /* D-pad */
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP))    input |= INPUT_UP;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN))  input |= INPUT_DOWN;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT))  input |= INPUT_LEFT;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) input |= INPUT_RIGHT;
            /* Action buttons from bindings */
            for (int i = 0; i < REMAP_BUTTON_COUNT; i++) {
                if (b->buttons[i].gamepad_button >= 0 &&
                    IsGamepadButtonDown(gamepad, b->buttons[i].gamepad_button))
                    input |= binding_bits[i];
            }
            /* Bumpers: RB=Heavy (alt), LB=S (alt) — always hardcoded */
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) input |= INPUT_HEAVY;
            if (IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_TRIGGER_1))  input |= INPUT_SPECIAL;
        }
    }

    return input;
}

const char *input_key_name(int key) {
    switch (key) {
        case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
        case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
        case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
        case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
        case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
        case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
        case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
        case KEY_Y: return "Y"; case KEY_Z: return "Z";
        case KEY_ZERO: return "0"; case KEY_ONE: return "1"; case KEY_TWO: return "2";
        case KEY_THREE: return "3"; case KEY_FOUR: return "4"; case KEY_FIVE: return "5";
        case KEY_SIX: return "6"; case KEY_SEVEN: return "7"; case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";
        case KEY_KP_0: return "KP0"; case KEY_KP_1: return "KP1"; case KEY_KP_2: return "KP2";
        case KEY_KP_3: return "KP3"; case KEY_KP_4: return "KP4"; case KEY_KP_5: return "KP5";
        case KEY_KP_6: return "KP6"; case KEY_KP_7: return "KP7"; case KEY_KP_8: return "KP8";
        case KEY_KP_9: return "KP9";
        case KEY_SPACE: return "Space";
        case KEY_LEFT_SHIFT: return "LShift"; case KEY_RIGHT_SHIFT: return "RShift";
        case KEY_LEFT_CONTROL: return "LCtrl"; case KEY_RIGHT_CONTROL: return "RCtrl";
        case KEY_LEFT_ALT: return "LAlt"; case KEY_RIGHT_ALT: return "RAlt";
        case KEY_TAB: return "Tab"; case KEY_BACKSPACE: return "Bksp";
        case KEY_SEMICOLON: return ";"; case KEY_APOSTROPHE: return "'";
        case KEY_COMMA: return ","; case KEY_PERIOD: return ".";
        case KEY_SLASH: return "/"; case KEY_BACKSLASH: return "\\";
        case KEY_LEFT_BRACKET: return "["; case KEY_RIGHT_BRACKET: return "]";
        case KEY_MINUS: return "-"; case KEY_EQUAL: return "=";
        default: return "???";
    }
}

const char *input_gamepad_button_name(int button) {
    switch (button) {
        case GAMEPAD_BUTTON_RIGHT_FACE_LEFT:  return "X";
        case GAMEPAD_BUTTON_RIGHT_FACE_UP:    return "Y";
        case GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: return "B";
        case GAMEPAD_BUTTON_RIGHT_FACE_DOWN:  return "A";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_1:   return "LB";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_1:  return "RB";
        case GAMEPAD_BUTTON_LEFT_TRIGGER_2:   return "LT";
        case GAMEPAD_BUTTON_RIGHT_TRIGGER_2:  return "RT";
        case GAMEPAD_BUTTON_MIDDLE_LEFT:      return "Back";
        case GAMEPAD_BUTTON_MIDDLE_RIGHT:     return "Start";
        case GAMEPAD_BUTTON_LEFT_THUMB:       return "LS";
        case GAMEPAD_BUTTON_RIGHT_THUMB:      return "RS";
        default: return "???";
    }
}

/* Check if a key is reserved (not remappable) */
static int is_reserved_key(int key) {
    return key == KEY_W || key == KEY_A || key == KEY_S || key == KEY_D ||
           key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT ||
           key == KEY_ENTER || key == KEY_ESCAPE || key == KEY_F1 || key == KEY_F2;
}

/* Check if a gamepad button is reserved (d-pad, start) */
static int is_reserved_gamepad(int button) {
    return button == GAMEPAD_BUTTON_LEFT_FACE_UP ||
           button == GAMEPAD_BUTTON_LEFT_FACE_DOWN ||
           button == GAMEPAD_BUTTON_LEFT_FACE_LEFT ||
           button == GAMEPAD_BUTTON_LEFT_FACE_RIGHT ||
           button == GAMEPAD_BUTTON_MIDDLE_RIGHT;
}

/* Scan for first newly pressed keyboard key. Returns -1 if none. */
int input_scan_key(void) {
    /* Scan common keys */
    for (int k = KEY_SPACE; k <= KEY_Z; k++) {
        if (IsKeyPressed(k) && !is_reserved_key(k)) return k;
    }
    for (int k = KEY_KP_0; k <= KEY_KP_9; k++) {
        if (IsKeyPressed(k)) return k;
    }
    /* Additional keys */
    int extras[] = { KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT, KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL,
                     KEY_LEFT_ALT, KEY_RIGHT_ALT, KEY_TAB, KEY_BACKSPACE,
                     KEY_SEMICOLON, KEY_APOSTROPHE, KEY_COMMA, KEY_PERIOD,
                     KEY_SLASH, KEY_BACKSLASH, KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET,
                     KEY_MINUS, KEY_EQUAL };
    for (int i = 0; i < (int)(sizeof(extras)/sizeof(extras[0])); i++) {
        if (IsKeyPressed(extras[i]) && !is_reserved_key(extras[i])) return extras[i];
    }
    return -1;
}

/* Scan for first newly pressed gamepad button. Returns -1 if none. */
int input_scan_gamepad(int gamepad) {
    if (!IsGamepadAvailable(gamepad)) return -1;
    int buttons[] = {
        GAMEPAD_BUTTON_RIGHT_FACE_LEFT, GAMEPAD_BUTTON_RIGHT_FACE_UP,
        GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, GAMEPAD_BUTTON_RIGHT_FACE_DOWN,
        GAMEPAD_BUTTON_LEFT_TRIGGER_1, GAMEPAD_BUTTON_RIGHT_TRIGGER_1,
        GAMEPAD_BUTTON_LEFT_TRIGGER_2, GAMEPAD_BUTTON_RIGHT_TRIGGER_2,
        GAMEPAD_BUTTON_LEFT_THUMB, GAMEPAD_BUTTON_RIGHT_THUMB,
        GAMEPAD_BUTTON_MIDDLE_LEFT
    };
    for (int i = 0; i < (int)(sizeof(buttons)/sizeof(buttons[0])); i++) {
        if (IsGamepadButtonPressed(gamepad, buttons[i]) && !is_reserved_gamepad(buttons[i]))
            return buttons[i];
    }
    return -1;
}

#endif /* TESTING_HEADLESS */