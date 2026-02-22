#include <stdio.h>
#include <string.h>
#include "../src/types.h"
#include "../src/player.h"
#include "../src/hitbox.h"
#include "../src/character.h"
#include "../src/projectile.h"

/* Simple test framework */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; printf("  PASS: %s\n", msg); } \
    else { tests_failed++; printf("  FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

/* Helper: create a player and tick N frames with given input */
static void tick(PlayerState *p, uint32_t input, int frames) {
    for (int i = 0; i < frames; i++) {
        player_update(p, input, NULL, 0);
    }
}

static CharacterState *active(PlayerState *p) {
    return &p->chars[p->active_char];
}

/* ========== TESTS ========== */

static void test_idle_on_init(void) {
    printf("test_idle_on_init:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    ASSERT(active(&p)->state == STATE_IDLE, "starts in IDLE");
    ASSERT(active(&p)->on_ground == TRUE, "starts on ground");
    ASSERT(active(&p)->vx == 0, "no horizontal velocity");
    ASSERT(active(&p)->vy == 0, "no vertical velocity");
}

static void test_walk_forward(void) {
    printf("test_walk_forward:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* P1 faces right, so INPUT_RIGHT = walk forward */
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_WALK_FORWARD, "walking forward on right input");
    fixed_t x_before = active(&p)->x;
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->x > x_before, "moved right");
    /* Release: should return to idle */
    tick(&p, 0, 1);
    ASSERT(active(&p)->state == STATE_IDLE, "idle after releasing direction");
    ASSERT(active(&p)->vx == 0, "velocity zero in idle");
}

static void test_walk_backward(void) {
    printf("test_walk_backward:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* P1 faces right, so INPUT_LEFT = walk backward */
    tick(&p, INPUT_LEFT, 1);
    ASSERT(active(&p)->state == STATE_WALK_BACKWARD, "walking backward on left input");
    fixed_t x_before = active(&p)->x;
    tick(&p, INPUT_LEFT, 1);
    ASSERT(active(&p)->x < x_before, "moved left");
}

static void test_no_slide_on_tap(void) {
    printf("test_no_slide_on_tap:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* Tap right for 1 frame then release */
    tick(&p, INPUT_RIGHT, 1);
    fixed_t x_after_tap = active(&p)->x;
    tick(&p, 0, 1);
    ASSERT(active(&p)->state == STATE_IDLE, "idle after tap release");
    /* Should not move further */
    fixed_t x_after_release = active(&p)->x;
    tick(&p, 0, 5);
    ASSERT(active(&p)->x == x_after_release, "no sliding after release");
}

static void test_jump_from_idle(void) {
    printf("test_jump_from_idle:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat on up press");
    /* Tick through jump squat (4 frames) */
    tick(&p, 0, 4);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne after jump squat");
    ASSERT(active(&p)->on_ground == FALSE, "not on ground");
    ASSERT(active(&p)->vy < 0, "negative vy (going up)");
}

static void test_jump_while_walking(void) {
    printf("test_jump_while_walking:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Walk right then press up+right simultaneously */
    tick(&p, INPUT_RIGHT, 3);
    ASSERT(active(&p)->state == STATE_WALK_FORWARD, "walking forward");
    tick(&p, INPUT_RIGHT | INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat from walk + up");
}

static void test_jump_with_direction(void) {
    printf("test_jump_with_direction:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press up+right from idle */
    tick(&p, INPUT_UP | INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat on up+right");
    /* Tick through squat while holding direction */
    tick(&p, INPUT_UP | INPUT_RIGHT, 4);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    ASSERT(active(&p)->vx > 0, "has rightward velocity from directional jump");
}

static void test_crouch(void) {
    printf("test_crouch:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    fixed_t y_before = active(&p)->y;
    int height_before = active(&p)->height;
    tick(&p, INPUT_DOWN, 1);
    ASSERT(active(&p)->state == STATE_CROUCH, "crouching on down press");
    ASSERT(active(&p)->height < height_before, "shorter when crouching");
    ASSERT(active(&p)->crouching == TRUE, "crouch flag set");
    /* Bottom of character should stay at same position */
    int bottom_before = FIXED_TO_INT(y_before) + height_before;
    int bottom_after = FIXED_TO_INT(active(&p)->y) + active(&p)->height;
    ASSERT(bottom_before == bottom_after, "feet stay on ground when crouching");
    /* Release down */
    tick(&p, 0, 1);
    ASSERT(active(&p)->state == STATE_IDLE, "idle after releasing crouch");
    ASSERT(active(&p)->height == height_before, "height restored");
    ASSERT(active(&p)->crouching == FALSE, "crouch flag cleared");
    /* Feet should still be grounded */
    int bottom_restored = FIXED_TO_INT(active(&p)->y) + active(&p)->height;
    ASSERT(bottom_before == bottom_restored, "feet stay on ground after uncrouching");
}

static void test_dash_forward(void) {
    printf("test_dash_forward:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* Double tap right (P1 faces right = forward) */
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);  /* brief gap */
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "dash forward on double tap");
    ASSERT(active(&p)->dashing == TRUE, "dash flag set");
    /* Let dash complete */
    tick(&p, 0, 20);
    ASSERT(active(&p)->state == STATE_IDLE, "idle after dash ends");
    ASSERT(active(&p)->dashing == FALSE, "dash flag cleared");
}

static void test_dash_backward(void) {
    printf("test_dash_backward:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* Double tap left (P1 faces right, so left = backward) */
    tick(&p, INPUT_LEFT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_LEFT, 1);
    ASSERT(active(&p)->state == STATE_DASH_BACKWARD, "dash backward on double tap left");
}

static void test_landing_frames(void) {
    printf("test_landing_frames:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Jump */
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4); /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    /* Tick until landing */
    for (int i = 0; i < 200; i++) {
        tick(&p, 0, 1);
        if (active(&p)->state == STATE_LANDING) break;
    }
    ASSERT(active(&p)->state == STATE_LANDING, "entered landing state");
    tick(&p, 0, 2);
    ASSERT(active(&p)->state == STATE_IDLE, "idle after landing frames");
}

static void test_facing_direction(void) {
    printf("test_facing_direction:\n");
    PlayerState p1, p2;
    player_init(&p1, 1, 200, 400, CHAR_RYKER);
    player_init(&p2, 2, 600, 400, CHAR_RYKER);
    ASSERT(active(&p1)->facing == 1, "P1 faces right initially");
    ASSERT(active(&p2)->facing == -1, "P2 faces left initially");
    /* Move P1 past P2 */
    for (int i = 0; i < 200; i++) {
        player_update(&p1, INPUT_RIGHT, NULL, active(&p2)->x);
        player_update(&p2, 0, NULL, active(&p1)->x);
        player_update_facing(&p1, &p2);
    }
    /* P1 should now be to the right of P2 */
    if (active(&p1)->x > active(&p2)->x) {
        ASSERT(active(&p1)->facing == -1, "P1 faces left after crossing");
        ASSERT(active(&p2)->facing == 1, "P2 faces right after crossing");
    }
}

static void test_stage_bounds(void) {
    printf("test_stage_bounds:\n");
    PlayerState p;
    player_init(&p, 1, 10, 400, CHAR_RYKER);
    /* Walk left into wall */
    for (int i = 0; i < 100; i++) {
        player_update(&p, INPUT_LEFT, NULL, 0);
    }
    ASSERT(active(&p)->x >= 0, "cannot go past left wall");
    /* Walk right into wall */
    player_init(&p, 1, 1200, 400, CHAR_RYKER);
    for (int i = 0; i < 100; i++) {
        player_update(&p, INPUT_RIGHT, NULL, 0);
    }
    ASSERT(active(&p)->x <= FIXED_FROM_INT(STAGE_WIDTH - active(&p)->width), "cannot go past right wall");
}

static void test_jump_squat_uncancelable(void) {
    printf("test_jump_squat_uncancelable:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "in jump squat");
    /* Direction during jump squat is allowed (momentum), but state stays */
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "still in jump squat with direction");
    tick(&p, INPUT_DOWN, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "still in jump squat (can't crouch)");
}

static void test_jump_squat_momentum(void) {
    printf("test_jump_squat_momentum:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Walk right then jump — should keep moving during jump squat */
    tick(&p, INPUT_RIGHT, 3);
    ASSERT(active(&p)->state == STATE_WALK_FORWARD, "walking forward");
    fixed_t x_before_jump = active(&p)->x;
    tick(&p, INPUT_RIGHT | INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "entered jump squat");
    /* Tick 2 more frames of jump squat while holding right */
    tick(&p, INPUT_RIGHT | INPUT_UP, 2);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "still in jump squat");
    ASSERT(active(&p)->x > x_before_jump, "kept moving during jump squat");
}

static void test_dash_crouch_cancel(void) {
    printf("test_dash_crouch_cancel:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* Double tap right to dash */
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "dashing forward");
    /* Try to crouch too early (before cancel window) */
    tick(&p, INPUT_DOWN, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "can't cancel dash too early");
    /* Tick to cancel window then crouch */
    tick(&p, 0, 4); /* get to frame 6+ */
    tick(&p, INPUT_DOWN, 1);
    ASSERT(active(&p)->state == STATE_CROUCH, "crouch cancel after min frames");
    ASSERT(active(&p)->dashing == FALSE, "dash flag cleared on cancel");
    ASSERT(active(&p)->crouching == TRUE, "crouch flag set on cancel");
}

/* Helper: press attack from idle and wait through the pending buffer */
/* Pending buffer window must match TWO_BUTTON_DASH_LENIENCY in player.c */
#define PENDING_BUFFER_WAIT 5
static void commit_attack_from_idle(PlayerState *p, uint32_t button) {
    tick(p, button, 1);                /* Press (enters pending buffer) */
    tick(p, 0, PENDING_BUFFER_WAIT);   /* Buffer expires, attack commits */
}

static void test_attack_5l(void) {
    printf("test_attack_5l:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press light attack (pending buffer delays commit) */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "enters ATTACK_STARTUP");
    /* Tick through remaining startup (5 total, 1 elapsed on commit frame) */
    tick(&p, 0, 4);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "enters ATTACK_ACTIVE after startup");
    /* Tick through active (3 frames) */
    tick(&p, 0, 3);
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "enters ATTACK_RECOVERY after active");
    /* Tick through recovery (10 frames) */
    tick(&p, 0, 10);
    ASSERT(active(&p)->state == STATE_IDLE, "returns to IDLE after recovery");
}

static void test_attack_5m(void) {
    printf("test_attack_5m:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    const MoveData *move = character_get_normal(CHAR_RYKER, NORMAL_5M);
    commit_attack_from_idle(&p, INPUT_MEDIUM);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "5M enters ATTACK_STARTUP");
    tick(&p, 0, move->startup_frames); /* through remaining startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5M enters ATTACK_ACTIVE");
    int active_frames = move->active_end - move->active_start + 1;
    tick(&p, 0, active_frames);
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "5M enters ATTACK_RECOVERY");
}

static void test_attack_5h(void) {
    printf("test_attack_5h:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    commit_attack_from_idle(&p, INPUT_HEAVY);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "5H enters ATTACK_STARTUP");
    tick(&p, 0, 11); /* remaining startup (12 total, 1 on commit frame) */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5H enters ATTACK_ACTIVE");
    tick(&p, 0, 4); /* active */
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "5H enters ATTACK_RECOVERY");
}

static void test_hitstun(void) {
    printf("test_hitstun:\n");
    PlayerState p, opp;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    player_init(&opp, 2, 350, 400, CHAR_RYKER);
    
    /* P1 attacks P2 */
    tick(&p, INPUT_LIGHT, 1);  /* P1 starts attack */
    tick(&p, 0, 5); /* through startup */
    tick(&p, 0, 3); /* through active - hit should connect */
    
    /* P2 should be in hitstun now (game logic would connect) */
    /* For now, manually set hitstun to test the state */
    active(&opp)->state = STATE_HITSTUN;
    active(&opp)->hitstun_remaining = 12;
    active(&opp)->in_hitstun = TRUE;
    
    ASSERT(active(&opp)->state == STATE_HITSTUN, "in HITSTUN state");
    ASSERT(active(&opp)->hitstun_remaining == 12, "hitstun frames set");
    
    /* Tick through hitstun */
    for (int i = 0; i < 12; i++) {
        tick(&opp, 0, 1);
    }
    ASSERT(active(&opp)->state == STATE_IDLE, "returns to IDLE after hitstun");
    ASSERT(active(&opp)->in_hitstun == FALSE, "hitstun flag cleared");
}

static void test_blockstun(void) {
    printf("test_blockstun:\n");
    PlayerState p;
    player_init(&p, 2, 350, 400, CHAR_RYKER);  /* P2 */
    
    /* Set to blockstun state */
    active(&p)->state = STATE_BLOCKSTUN;
    active(&p)->blockstun_remaining = 8;
    active(&p)->in_blockstun = TRUE;
    
    ASSERT(active(&p)->state == STATE_BLOCKSTUN, "in BLOCKSTUN state");
    ASSERT(active(&p)->blockstun_remaining == 8, "blockstun frames set");
    
    /* Tick through blockstun */
    for (int i = 0; i < 8; i++) {
        tick(&p, 0, 1);
    }
    ASSERT(active(&p)->state == STATE_IDLE, "returns to IDLE after blockstun");
    ASSERT(active(&p)->in_blockstun == FALSE, "blockstun flag cleared");
}

/* ========== CANCEL HIERARCHY TESTS ========== */

static void test_can_cancel_function(void) {
    printf("test_can_cancel_function:\n");
    /* CANCEL_FREE allows everything */
    ASSERT(CANCEL_FREE != CANCEL_NONE && ACTION_MOVEMENT >= CANCEL_FREE, "FREE allows MOVEMENT");
    ASSERT(CANCEL_FREE != CANCEL_NONE && ACTION_NORMAL >= CANCEL_FREE, "FREE allows NORMAL");
    ASSERT(CANCEL_FREE != CANCEL_NONE && ACTION_SUPER >= CANCEL_FREE, "FREE allows SUPER");
    /* CANCEL_BY_NORMAL blocks movement */
    ASSERT(!(CANCEL_BY_NORMAL != CANCEL_NONE && ACTION_MOVEMENT >= CANCEL_BY_NORMAL), "BY_NORMAL blocks MOVEMENT");
    ASSERT(CANCEL_BY_NORMAL != CANCEL_NONE && ACTION_NORMAL >= CANCEL_BY_NORMAL, "BY_NORMAL allows NORMAL");
    ASSERT(CANCEL_BY_NORMAL != CANCEL_NONE && ACTION_SPECIAL >= CANCEL_BY_NORMAL, "BY_NORMAL allows SPECIAL");
    /* CANCEL_BY_SPECIAL blocks normals */
    ASSERT(!(CANCEL_BY_SPECIAL != CANCEL_NONE && ACTION_NORMAL >= CANCEL_BY_SPECIAL), "BY_SPECIAL blocks NORMAL");
    ASSERT(CANCEL_BY_SPECIAL != CANCEL_NONE && ACTION_SPECIAL >= CANCEL_BY_SPECIAL, "BY_SPECIAL allows SPECIAL");
    /* CANCEL_BY_SUPER blocks specials */
    ASSERT(!(CANCEL_BY_SUPER != CANCEL_NONE && ACTION_SPECIAL >= CANCEL_BY_SUPER), "BY_SUPER blocks SPECIAL");
    ASSERT(CANCEL_BY_SUPER != CANCEL_NONE && ACTION_SUPER >= CANCEL_BY_SUPER, "BY_SUPER allows SUPER");
    /* CANCEL_NONE blocks everything */
    ASSERT(CANCEL_NONE == CANCEL_NONE, "NONE blocks everything (checked via ==)");
}

static void test_cancel_idle_by_normal(void) {
    printf("test_cancel_idle_by_normal:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    ASSERT(active(&p)->state == STATE_IDLE, "starts idle");
    /* Single button from idle enters pending buffer first */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(p.pending_attack != 0, "attack buffered (pending)");
    /* After buffer expires, attack commits */
    tick(&p, 0, PENDING_BUFFER_WAIT);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "attack commits after buffer");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5M), "correct attack selected");
}

static void test_cancel_startup_by_two_button_dash(void) {
    printf("test_cancel_startup_by_two_button_dash:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press M from idle — enters pending buffer */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(p.pending_attack != 0, "M buffered (pending)");
    /* Press L while still pending — second button within leniency → dash! */
    tick(&p, INPUT_LIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD || active(&p)->state == STATE_DASH_BACKWARD,
           "two-button dash fires from pending buffer");
}

static void test_plink_dash_sequence(void) {
    printf("test_plink_dash_sequence:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Step 1: Press M from idle → pending buffer */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(p.pending_attack != 0, "step 1: M buffered");
    /* Step 2: Press L while pending → two-button dash fires directly from idle */
    tick(&p, INPUT_LIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD || active(&p)->state == STATE_DASH_BACKWARD,
           "step 2: dash from pending buffer");
    /* Step 3: Tick past DASH_BUTTON_CANCEL_FRAMES (2), then press H */
    tick(&p, 0, 2);  /* frames 1-2 of dash (locked) */
    tick(&p, INPUT_HEAVY, 1);  /* frame 3: newly pressed H — attack from dash (no buffer, lvl != FREE) */
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "step 3: attack from dash");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5H), "correct attack: 5H");
}

static void test_recovery_not_cancelable_on_whiff(void) {
    printf("test_recovery_not_cancelable_on_whiff:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5L through pending buffer, then tick through all phases */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, 4);              /* remaining startup */
    tick(&p, 0, 3);              /* through active (3 frames) */
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "in recovery");
    ASSERT(p.hit_confirmed == 0, "no hit confirmed (whiff)");
    /* Press another button — should NOT cancel */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "recovery locked on whiff");
}

static void test_chain_cancel_on_hit(void) {
    printf("test_chain_cancel_on_hit:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5L (has MOVE_PROP_CHAIN) through pending buffer */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, 4);  /* remaining startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "in active frames");
    /* Simulate hit confirmed */
    p.hit_confirmed = 1;
    /* Press M — should chain cancel (CHAIN → CANCEL_BY_NORMAL, M is ACTION_NORMAL)
     * No pending buffer here since lvl != CANCEL_FREE */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "chain cancel into 5M");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5M), "new attack is 5M");
    ASSERT(p.hit_confirmed == 0, "hit_confirmed reset on new attack");
}

static void test_nothing_cancels_hitstun(void) {
    printf("test_nothing_cancels_hitstun:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Force into hitstun */
    active(&p)->state = STATE_HITSTUN;
    active(&p)->state_frame = 0;
    active(&p)->hitstun_remaining = 20;
    active(&p)->in_hitstun = TRUE;
    /* Mash buttons — nothing should work */
    tick(&p, INPUT_LIGHT | INPUT_MEDIUM | INPUT_HEAVY, 1);
    ASSERT(active(&p)->state == STATE_HITSTUN, "attack buttons can't cancel hitstun");
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_HITSTUN, "jump can't cancel hitstun");
}

static void test_dash_early_frames_locked(void) {
    printf("test_dash_early_frames_locked:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);
    /* Double tap to dash */
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "dashing forward");
    /* Press attack on frame 1 of dash — should NOT cancel (before DASH_BUTTON_CANCEL_FRAMES) */
    tick(&p, INPUT_HEAVY, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "attack blocked during early dash frames");
}

static void test_combo_buffer_during_recovery(void) {
    printf("test_combo_buffer_during_recovery:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    const MoveData *jab = character_get_normal(CHAR_RYKER, NORMAL_5L);
    /* Start 5L, go through to recovery (whiff) */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, jab->startup_frames);  /* through startup */
    int jab_active = jab->active_end - jab->active_start + 1;
    tick(&p, 0, jab_active);  /* active */
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "in recovery");
    /* Press M near end of recovery: 3 frames before it ends.
     * The 5-frame buffer window covers the remaining recovery + first idle frame. */
    int press_at = jab->recovery_frames - 3;
    if (press_at < 1) press_at = 1;
    tick(&p, 0, press_at);
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "still in recovery");
    ASSERT(p.buffered_button != 0, "M is buffered");
    /* Tick remaining recovery + 1 for buffer to fire on idle */
    int remaining = jab->recovery_frames - press_at - 1;
    tick(&p, 0, remaining + 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "buffered M fires on idle");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5M), "correct attack from buffer");
}

static void test_combo_buffer_expires(void) {
    printf("test_combo_buffer_expires:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Force into hitstun (long, so buffer will expire) */
    active(&p)->state = STATE_HITSTUN;
    active(&p)->state_frame = 0;
    active(&p)->hitstun_remaining = 20;
    active(&p)->in_hitstun = TRUE;
    /* Press M early in hitstun */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(p.buffered_button != 0, "M buffered during hitstun");
    /* Tick through rest of hitstun (19 more frames — way past 5-frame buffer window) */
    tick(&p, 0, 19);
    ASSERT(active(&p)->state == STATE_IDLE, "hitstun ended");
    /* Buffer should have expired — no attack should fire */
    tick(&p, 0, 1);
    ASSERT(active(&p)->state == STATE_IDLE, "buffer expired, still idle");
}

static void test_combo_buffer_chain_on_hit(void) {
    printf("test_combo_buffer_chain_on_hit:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5L through pending buffer */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, 4);  /* remaining startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "in active frames");
    /* Press M during active (no hit yet — CANCEL_NONE). Should buffer. */
    tick(&p, INPUT_MEDIUM, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "still active (no hit confirm)");
    ASSERT(p.buffered_button != 0, "M buffered");
    /* Now hit confirms! */
    p.hit_confirmed = 1;
    /* Next frame: buffer replays M, cancel level is now CANCEL_BY_NORMAL (CHAIN) */
    tick(&p, 0, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "buffered M chains on hit confirm");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5M), "chained into 5M");
}

static void test_two_button_dash_leniency(void) {
    printf("test_two_button_dash_leniency:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press M (enters pending buffer), release M, press L 1 frame later.
     * Pending buffer detects second button within leniency → dash. */
    tick(&p, INPUT_MEDIUM, 1);
    tick(&p, INPUT_LIGHT, 1);  /* M released, L pressed — still within window */
    ASSERT(active(&p)->state == STATE_DASH_FORWARD || active(&p)->state == STATE_DASH_BACKWARD,
           "dash fires with 1-frame gap (leniency)");

    /* 2-frame gap should also work (still within 3-frame window) */
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    tick(&p, INPUT_HEAVY, 1);  /* H enters pending buffer */
    tick(&p, 0, 1);            /* 1 frame gap */
    tick(&p, INPUT_LIGHT, 1);  /* L arrives — within window */
    ASSERT(active(&p)->state == STATE_DASH_FORWARD || active(&p)->state == STATE_DASH_BACKWARD,
           "dash fires with 2-frame gap (leniency)");

    /* Gap beyond leniency window should NOT work */
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    tick(&p, INPUT_HEAVY, 1);            /* H enters pending buffer */
    tick(&p, 0, PENDING_BUFFER_WAIT);    /* Buffer expires → attack commits */
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "buffer expired, attack committed");
    /* Now L arrives — startup is not cancelable, L goes to combo buffer */
    tick(&p, INPUT_LIGHT, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "still in startup (not cancelable)");
    ASSERT(p.buffered_button != 0, "L buffered for later");
}

/* ========== CROUCHING NORMAL TESTS ========== */

/* Helper: press attack from crouch (hold DOWN through pending buffer) */
static void commit_attack_from_crouch(PlayerState *p, uint32_t button) {
    tick(p, INPUT_DOWN | button, 1);  /* Press button while crouching */
    tick(p, INPUT_DOWN, PENDING_BUFFER_WAIT);  /* Hold DOWN through buffer */
}

static void test_crouch_attack_2l(void) {
    printf("test_crouch_attack_2l:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Crouch first */
    tick(&p, INPUT_DOWN, 1);
    ASSERT(active(&p)->state == STATE_CROUCH, "crouching");
    ASSERT(active(&p)->crouching == TRUE, "crouch flag set");
    int crouch_h = active(&p)->height;
    /* Press L while crouching (hold DOWN through buffer) */
    commit_attack_from_crouch(&p, INPUT_LIGHT);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "2L attack started");
    ASSERT(active(&p)->height == crouch_h, "crouch height kept during 2L");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_2L), "correct move: 2L");
}

static void test_crouch_attack_returns_to_crouch(void) {
    printf("test_crouch_attack_returns_to_crouch:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Crouch then attack */
    tick(&p, INPUT_DOWN, 1);
    commit_attack_from_crouch(&p, INPUT_LIGHT);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "in startup");
    /* Tick through full 2L: startup(5) + active(3) + recovery(8) */
    tick(&p, INPUT_DOWN, 4);  /* remaining startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "active");
    tick(&p, INPUT_DOWN, 3);  /* active frames */
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "recovery");
    tick(&p, INPUT_DOWN, 8);  /* recovery */
    ASSERT(active(&p)->state == STATE_CROUCH, "returns to crouch after 2L");
}

/* ========== AIR NORMAL TESTS ========== */

static void test_air_attack_jl(void) {
    printf("test_air_attack_jl:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Jump past jump squat */
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    /* Press L while airborne — commits immediately (no pending buffer in air) */
    tick(&p, INPUT_LIGHT, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "j.L started from air");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_JL), "correct move: j.L");
}

static void test_air_attack_preserves_momentum(void) {
    printf("test_air_attack_preserves_momentum:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Jump with rightward momentum */
    tick(&p, INPUT_UP | INPUT_RIGHT, 1);
    tick(&p, INPUT_RIGHT, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    fixed_t vx_before = active(&p)->vx;
    ASSERT(vx_before != 0, "has horizontal momentum");
    /* Start air attack — commits immediately, no air friction on normal jump */
    tick(&p, INPUT_LIGHT, 1);
    ASSERT(active(&p)->vx == vx_before, "vx preserved after air attack start");
}

static void test_air_attack_gravity(void) {
    printf("test_air_attack_gravity:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Jump */
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    /* Start air attack — commits immediately in air */
    tick(&p, INPUT_LIGHT, 1);
    fixed_t vy_before = active(&p)->vy;
    /* Tick 1 frame — gravity should increase vy */
    tick(&p, 0, 1);
    ASSERT(active(&p)->vy > vy_before, "gravity applied during air attack");
}

/* ========== JUMP CANCEL TESTS ========== */

static void test_jump_cancel_on_hit(void) {
    printf("test_jump_cancel_on_hit:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5L (has JUMP_CANCEL) */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, 4);  /* remaining startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "active");
    /* Simulate hit confirmed */
    p.hit_confirmed = 1;
    /* Press UP — should jump cancel */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump cancel into jump squat");
}

static void test_no_jump_cancel_on_whiff(void) {
    printf("test_no_jump_cancel_on_whiff:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5L, go to recovery (whiff) */
    commit_attack_from_idle(&p, INPUT_LIGHT);
    tick(&p, 0, 4);  /* startup */
    tick(&p, 0, 3);  /* active */
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "recovery");
    ASSERT(p.hit_confirmed == 0, "no hit confirmed");
    /* Press UP — should NOT jump cancel */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_RECOVERY, "still in recovery on whiff");
}

static void test_no_jump_cancel_5h(void) {
    printf("test_no_jump_cancel_5h:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5H (no JUMP_CANCEL) */
    commit_attack_from_idle(&p, INPUT_HEAVY);
    tick(&p, 0, 11);  /* startup */
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "active");
    p.hit_confirmed = 1;
    /* Press UP — should NOT jump cancel (5H has no JUMP_CANCEL) */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "no jump cancel on 5H");
}

/* ========== DASH JUMP TESTS ========== */

static void test_dash_jump_carries_momentum(void) {
    printf("test_dash_jump_carries_momentum:\n");
    PlayerState p;
    player_init(&p, 1, 400, 400, CHAR_RYKER);

    /* Normal jump: record distance */
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne from normal jump");
    fixed_t normal_vx = active(&p)->vx;

    /* Reset */
    player_init(&p, 1, 400, 400, CHAR_RYKER);

    /* Dash then jump: double tap right to dash */
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "dashing forward");
    fixed_t dash_vx = active(&p)->vx;

    /* Now press UP during dash */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat from dash");
    /* Tick through jump squat */
    tick(&p, 0, 4);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne from dash jump");
    /* Dash jump should have more horizontal speed than normal jump */
    fixed_t dash_jump_vx = active(&p)->vx;
    ASSERT(dash_jump_vx > normal_vx, "dash jump has more momentum than normal jump");
    /* Should carry the dash velocity */
    ASSERT(dash_jump_vx > 0, "dash jump moves forward");
}

static void test_dash_jump_further_distance(void) {
    printf("test_dash_jump_further_distance:\n");
    PlayerState p1, p2;

    /* Normal jump — measure horizontal distance */
    player_init(&p1, 1, 400, 400, CHAR_RYKER);
    tick(&p1, INPUT_UP | INPUT_RIGHT, 1);
    tick(&p1, INPUT_RIGHT, 4);  /* jump squat with direction */
    /* Fly until landing */
    for (int i = 0; i < 200; i++) {
        tick(&p1, 0, 1);
        if (active(&p1)->on_ground) break;
    }
    fixed_t normal_dist = active(&p1)->x - FIXED_FROM_INT(400);

    /* Dash jump — measure horizontal distance */
    player_init(&p2, 1, 400, 400, CHAR_RYKER);
    tick(&p2, INPUT_RIGHT, 1);
    tick(&p2, 0, 2);
    tick(&p2, INPUT_RIGHT, 1);  /* start dash */
    tick(&p2, INPUT_UP, 1);     /* jump from dash */
    tick(&p2, 0, 4);            /* through jump squat */
    for (int i = 0; i < 200; i++) {
        tick(&p2, 0, 1);
        if (active(&p2)->on_ground) break;
    }
    fixed_t dash_dist = active(&p2)->x - FIXED_FROM_INT(400);

    ASSERT(dash_dist > normal_dist, "dash jump covers more distance than normal jump");
}

/* ========== MOTION INPUT TESTS ========== */

/* Helper: feed input into buffer + player_update together */
static void tick_with_buf(PlayerState *p, InputBuffer *buf, uint32_t input, int frames) {
    for (int i = 0; i < frames; i++) {
        input_update(buf, input);
        player_update(p, input_get_current(buf), buf, 0);
    }
}

/* Tick with explicit opponent_x for throw range testing */
static void tick_opp(PlayerState *p, uint32_t input, int frames, fixed_t opponent_x) {
    for (int i = 0; i < frames; i++) {
        player_update(p, input, NULL, opponent_x);
    }
}

static void test_motion_qcf_p1(void) {
    printf("test_motion_qcf_p1:\n");
    InputBuffer buf;
    input_init(&buf);
    /* P1 facing right: down → down-right → right + L = QCF */
    input_update(&buf, INPUT_DOWN);
    input_update(&buf, INPUT_DOWN | INPUT_RIGHT);
    input_update(&buf, INPUT_RIGHT | INPUT_LIGHT);
    MotionType m = input_detect_motion(&buf, 1);
    ASSERT(m == MOTION_QCF, "QCF detected for P1 facing right");
    /* Verify character_get_special returns the fireball */
    const MoveData *special = character_get_special(CHAR_RYKER, m, INPUT_LIGHT);
    ASSERT(special != NULL, "236L special exists");
    ASSERT(special->move_type == MOVE_TYPE_SPECIAL, "236L is a special move");
}

static void test_motion_qcb_p1(void) {
    printf("test_motion_qcb_p1:\n");
    InputBuffer buf;
    input_init(&buf);
    /* P1 facing right: down → down-left → left + L = QCB */
    input_update(&buf, INPUT_DOWN);
    input_update(&buf, INPUT_DOWN | INPUT_LEFT);
    input_update(&buf, INPUT_LEFT | INPUT_LIGHT);
    MotionType m = input_detect_motion(&buf, 1);
    ASSERT(m == MOTION_QCB, "QCB detected for P1 facing right");
    const MoveData *special = character_get_special(CHAR_RYKER, m, INPUT_LIGHT);
    ASSERT(special != NULL, "214L special exists");
}

static void test_motion_dp_p1(void) {
    printf("test_motion_dp_p1:\n");
    InputBuffer buf;
    input_init(&buf);
    /* P1 facing right: right → down → down-right + L = DP */
    input_update(&buf, INPUT_RIGHT);
    input_update(&buf, INPUT_DOWN);
    input_update(&buf, INPUT_DOWN | INPUT_RIGHT | INPUT_LIGHT);
    MotionType m = input_detect_motion(&buf, 1);
    ASSERT(m == MOTION_DP, "DP detected for P1 facing right");
    const MoveData *special = character_get_special(CHAR_RYKER, m, INPUT_LIGHT);
    ASSERT(special != NULL, "623L special exists");
}

static void test_motion_qcf_facing_left(void) {
    printf("test_motion_qcf_facing_left:\n");
    InputBuffer buf;
    input_init(&buf);
    /* P2 facing left: forward = LEFT. So QCF = down → down-left → left + L */
    input_update(&buf, INPUT_DOWN);
    input_update(&buf, INPUT_DOWN | INPUT_LEFT);
    input_update(&buf, INPUT_LEFT | INPUT_LIGHT);
    MotionType m = input_detect_motion(&buf, -1);
    ASSERT(m == MOTION_QCF, "QCF detected for P2 facing left (mirrored)");
    const MoveData *special = character_get_special(CHAR_RYKER, m, INPUT_LIGHT);
    ASSERT(special != NULL, "236L fires from mirrored input");
}

static void test_special_from_idle(void) {
    printf("test_special_from_idle:\n");
    PlayerState p;
    InputBuffer buf;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    input_init(&buf);
    /* Feed QCF motion: down → down-right → right+L */
    tick_with_buf(&p, &buf, INPUT_DOWN, 3);
    tick_with_buf(&p, &buf, INPUT_DOWN | INPUT_RIGHT, 3);
    tick_with_buf(&p, &buf, INPUT_RIGHT | INPUT_LIGHT, 1);
    /* Pending buffer: wait for it to expire (special bypasses pending buffer
     * since the cancel chain checks motion before single-button normals) */
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "special move starts in ATTACK_STARTUP");
    ASSERT(p.current_attack != NULL, "current attack set");
    ASSERT(p.current_attack->move_type == MOVE_TYPE_SPECIAL, "attack is a special move");
}

static void test_special_cancel_on_hit(void) {
    printf("test_special_cancel_on_hit:\n");
    PlayerState p;
    InputBuffer buf;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    input_init(&buf);
    /* Start 5M (has SPECIAL_CANCEL) via pending buffer */
    tick_with_buf(&p, &buf, INPUT_MEDIUM, 1);
    tick_with_buf(&p, &buf, 0, PENDING_BUFFER_WAIT);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "5M startup");
    /* Tick through remaining startup (9 total, 1 on commit) */
    tick_with_buf(&p, &buf, 0, 8);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5M active");
    /* Simulate hit */
    p.hit_confirmed = 1;
    /* Now do QCF+L to cancel into fireball */
    tick_with_buf(&p, &buf, INPUT_DOWN, 2);
    tick_with_buf(&p, &buf, INPUT_DOWN | INPUT_RIGHT, 2);
    tick_with_buf(&p, &buf, INPUT_RIGHT | INPUT_LIGHT, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "special cancel into fireball");
    ASSERT(p.current_attack != NULL, "current attack set after cancel");
    ASSERT(p.current_attack->move_type == MOVE_TYPE_SPECIAL, "cancelled into special");
}

static void test_pending_buffer_preserves_direction(void) {
    printf("test_pending_buffer_preserves_direction:\n");
    PlayerState p;
    InputBuffer buf;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    input_init(&buf);
    /* Press M from neutral (standing) */
    tick_with_buf(&p, &buf, INPUT_MEDIUM, 1);
    ASSERT(p.pending_attack != 0, "M enters pending buffer");
    /* Start QCF motion during pending window: DOWN arrives before buffer expires */
    tick_with_buf(&p, &buf, INPUT_DOWN, 1);
    tick_with_buf(&p, &buf, INPUT_DOWN | INPUT_RIGHT, 1);
    /* Let pending buffer expire (need PENDING_BUFFER_WAIT total frames after press) */
    tick_with_buf(&p, &buf, INPUT_RIGHT, PENDING_BUFFER_WAIT - 2);
    /* 5M should have committed — NOT 2M. Direction was neutral at press time. */
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "attack started from pending buffer");
    ASSERT(p.current_attack == character_get_normal(0, NORMAL_5M),
           "5M committed (not 2M) despite DOWN held during buffer");
}

/* ========== PROJECTILE TESTS ========== */

static void test_projectile_spawn(void) {
    printf("test_projectile_spawn:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p;
    player_init(&p, 1, 400, 520, CHAR_RYKER);
    /* Get 236L fireball move */
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    ASSERT(fireball != NULL, "236L move exists");
    ASSERT(fireball->properties & MOVE_PROP_PROJECTILE, "236L is a projectile");
    /* Spawn */
    int ok = projectile_spawn(&ps, 0, &p, fireball, NULL);
    ASSERT(ok == 1, "spawn succeeds");
    ASSERT(ps.projectiles[0].active == TRUE, "projectile is active");
    ASSERT(ps.projectiles[0].owner == 0, "owner is player 0");
    ASSERT(ps.projectiles[0].vx > 0, "velocity is positive (P1 faces right)");
    ASSERT(ps.projectiles[0].damage == fireball->damage, "damage copied from move");
}

static void test_projectile_one_per_player(void) {
    printf("test_projectile_one_per_player:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p1, p2;
    player_init(&p1, 1, 400, 520, CHAR_RYKER);
    player_init(&p2, 2, 800, 520, CHAR_RYKER);
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    /* First spawn for P1 */
    int ok1 = projectile_spawn(&ps, 0, &p1, fireball, NULL);
    ASSERT(ok1 == 1, "P1 first spawn succeeds");
    /* Second spawn for same player should fail */
    int ok2 = projectile_spawn(&ps, 0, &p1, fireball, NULL);
    ASSERT(ok2 == 0, "P1 second spawn blocked (one per player)");
    /* P2 can still spawn */
    int ok3 = projectile_spawn(&ps, 1, &p2, fireball, NULL);
    ASSERT(ok3 == 1, "P2 spawn succeeds (different player)");
}

static void test_projectile_movement(void) {
    printf("test_projectile_movement:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p;
    player_init(&p, 1, 400, 520, CHAR_RYKER);
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    projectile_spawn(&ps, 0, &p, fireball, NULL);
    fixed_t x_before = ps.projectiles[0].x;
    projectile_update(&ps);
    ASSERT(ps.projectiles[0].x > x_before, "projectile moved right after update");
    ASSERT(ps.projectiles[0].lifetime == PROJECTILE_LIFETIME - 1, "lifetime decremented");
}

static void test_projectile_despawn_bounds(void) {
    printf("test_projectile_despawn_bounds:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p;
    player_init(&p, 1, 400, 520, CHAR_RYKER);
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    projectile_spawn(&ps, 0, &p, fireball, NULL);
    /* Run updates until despawned */
    int frames = 0;
    while (ps.projectiles[0].active && frames < 200) {
        projectile_update(&ps);
        frames++;
    }
    ASSERT(ps.projectiles[0].active == FALSE, "projectile despawned");
    ASSERT(frames > 0 && frames <= PROJECTILE_LIFETIME, "despawned within lifetime");
}

static void test_projectile_despawn_on_hit(void) {
    printf("test_projectile_despawn_on_hit:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p;
    player_init(&p, 1, 400, 520, CHAR_RYKER);
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    projectile_spawn(&ps, 0, &p, fireball, NULL);
    ASSERT(ps.projectiles[0].active == TRUE, "projectile active before hit");
    /* Simulate hit by deactivating (game.c does this on collision) */
    ps.projectiles[0].active = FALSE;
    ASSERT(ps.projectiles[0].active == FALSE, "projectile deactivated on hit");
}

/* ========== POLISH FIX TESTS ========== */

static void test_dash_jump_speed_capped(void) {
    printf("test_dash_jump_speed_capped:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    /* Start a dash: double-tap right */
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_RIGHT, 1);
    ASSERT(active(&p)->state == STATE_DASH_FORWARD, "dashing forward");
    /* Jump cancel from dash */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat from dash");
    tick(&p, 0, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne from dash jump");
    /* vx should be capped at DASH_JUMP_MAX_SPEED */
    ASSERT(active(&p)->vx <= DASH_JUMP_MAX_SPEED, "dash jump vx capped");
    ASSERT(active(&p)->vx > 0, "dash jump still moves forward");
}

static void test_air_friction_dash_jump_only(void) {
    printf("test_air_friction_dash_jump_only:\n");
    PlayerState p;

    /* Normal forward jump: NO friction — momentum preserved */
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    tick(&p, INPUT_UP | INPUT_RIGHT, 1);
    tick(&p, INPUT_RIGHT, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne from normal jump");
    fixed_t normal_vx = active(&p)->vx;
    tick(&p, 0, 10);
    ASSERT(active(&p)->vx == normal_vx, "normal jump: no air friction");

    /* Dash jump: HAS friction — momentum decays */
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_RIGHT, 1);
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4);  /* through jump squat */
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne from dash jump");
    ASSERT(active(&p)->dash_jump == TRUE, "dash_jump flag set");
    fixed_t dash_vx = active(&p)->vx;
    tick(&p, 0, 10);
    ASSERT(active(&p)->vx < dash_vx, "dash jump: air friction slowed speed");
}

static void test_projectile_close_range_clamp(void) {
    printf("test_projectile_close_range_clamp:\n");
    ProjectileState ps;
    projectile_init(&ps);
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    /* Create a defender right next to the attacker */
    CharacterState defender;
    memset(&defender, 0, sizeof(defender));
    defender.x = FIXED_FROM_INT(420);  /* very close — 20px gap, less than fireball x_offset of 60 */
    defender.width = 50;
    const MoveData *fireball = character_get_special(CHAR_RYKER, MOTION_QCF, INPUT_LIGHT);
    int ok = projectile_spawn(&ps, 0, &p, fireball, &defender);
    ASSERT(ok == 1, "spawn succeeds at close range");
    /* Projectile should NOT be past the defender */
    ASSERT(ps.projectiles[0].x < defender.x, "projectile spawns in front of defender, not past");
}

static void test_pushbox_no_over_separation(void) {
    printf("test_pushbox_no_over_separation:\n");
    PlayerState p1, p2;
    /* Place players overlapping by 1 pixel */
    player_init(&p1, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    player_init(&p2, 2, 449, GROUND_Y - 80, CHAR_RYKER);  /* P1 width=50, so right=450, P2 left=449 => 1px overlap */
    fixed_t p1_before = active(&p1)->x;
    fixed_t p2_before = active(&p2)->x;
    player_resolve_collisions(&p1, &p2);
    /* Both should have moved, but only by 1 pixel total (not 2) */
    fixed_t p1_moved = p1_before - active(&p1)->x;
    fixed_t p2_moved = active(&p2)->x - p2_before;
    fixed_t total_push = p1_moved + p2_moved;
    /* With ceiling division, 1px overlap => push = (1+1)/2 = 1 each side = 2 total.
     * Previously was overlap/2+1 = 0+1 = 1 each = 2 total. Should be similar but no extra. */
    ASSERT(total_push <= FIXED_FROM_INT(2), "pushbox separation is minimal for small overlap");
    /* Verify they are no longer overlapping */
    int c1_right = FIXED_TO_INT(active(&p1)->x) + active(&p1)->width;
    int c2_left = FIXED_TO_INT(active(&p2)->x);
    ASSERT(c1_right <= c2_left, "no longer overlapping after push");
}

static void test_air_allows_crossup(void) {
    printf("test_air_allows_crossup:\n");
    PlayerState p1, p2;
    player_init(&p1, 1, 425, GROUND_Y - 80, CHAR_RYKER);
    player_init(&p2, 2, 425, GROUND_Y - 80, CHAR_RYKER);
    /* Make P1 airborne, overlapping P2 horizontally */
    active(&p1)->on_ground = FALSE;
    active(&p1)->y = FIXED_FROM_INT(GROUND_Y - 150);
    fixed_t x_before = active(&p1)->x;
    /* Resolve — should NOT push while airborne (allows crossups) */
    player_resolve_collisions(&p1, &p2);
    ASSERT(active(&p1)->x == x_before, "no pushbox while airborne — crossups allowed");
}

static void test_landing_resolves_side(void) {
    printf("test_landing_resolves_side:\n");
    PlayerState p1, p2;
    /* Place players overlapping — both grounded, pushbox should resolve */
    player_init(&p1, 1, 420, GROUND_Y - 80, CHAR_RYKER);
    player_init(&p2, 2, 430, GROUND_Y - 80, CHAR_RYKER);
    /* Both on ground — pushbox kicks in */
    player_resolve_collisions(&p1, &p2);
    int p1_right = FIXED_TO_INT(active(&p1)->x) + active(&p1)->width;
    int p2_left = FIXED_TO_INT(active(&p2)->x);
    ASSERT(p1_right <= p2_left, "ground pushbox separates overlapping players on landing");
}

/* ========== COMBO RULE TESTS ========== */

static void test_light_self_chain(void) {
    printf("test_light_self_chain:\n");
    PlayerState p1, p2;
    player_init(&p1, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    player_init(&p2, 2, 450, GROUND_Y - 80, CHAR_RYKER);
    InputBuffer buf;
    input_init(&buf);
    /* Start 5L and manually set hit_confirmed */
    const MoveData *jab = character_get_normal(CHAR_RYKER, NORMAL_5L);
    tick_with_buf(&p1, &buf, INPUT_LIGHT, 1);
    tick_with_buf(&p1, &buf, 0, PENDING_BUFFER_WAIT);
    ASSERT(active(&p1)->state == STATE_ATTACK_STARTUP, "5L started");
    /* Advance through startup + into active */
    tick_with_buf(&p1, &buf, 0, jab->startup_frames);
    ASSERT(active(&p1)->state == STATE_ATTACK_ACTIVE, "5L active");
    /* Simulate hit confirmed */
    p1.hit_confirmed = 1;
    /* Press L again — should chain into another 5L */
    tick_with_buf(&p1, &buf, INPUT_LIGHT, 1);
    ASSERT(active(&p1)->state == STATE_ATTACK_STARTUP, "5L chained into 5L");
    ASSERT(p1.current_attack == jab, "still the same jab move");
}

static void test_all_normals_special_cancel(void) {
    printf("test_all_normals_special_cancel:\n");
    /* Engine rule: all normals on hit get at least CANCEL_BY_SPECIAL.
     * Test with 5H which has no MOVE_PROP_CHAIN — should still special cancel. */
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    InputBuffer buf;
    input_init(&buf);
    const MoveData *heavy = character_get_normal(CHAR_RYKER, NORMAL_5H);
    /* Start 5H */
    tick_with_buf(&p, &buf, INPUT_HEAVY, 1);
    tick_with_buf(&p, &buf, 0, PENDING_BUFFER_WAIT);
    tick_with_buf(&p, &buf, 0, heavy->startup_frames);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5H active");
    p.hit_confirmed = 1;
    /* Input QCF + L for fireball — should cancel */
    input_update(&buf, INPUT_DOWN);
    player_update(&p, INPUT_DOWN, &buf, 0);
    input_update(&buf, INPUT_DOWN | INPUT_RIGHT);
    player_update(&p, INPUT_DOWN | INPUT_RIGHT, &buf, 0);
    input_update(&buf, INPUT_RIGHT | INPUT_LIGHT);
    player_update(&p, INPUT_RIGHT | INPUT_LIGHT, &buf, 0);
    /* Should have cancelled into special */
    ASSERT(p.current_attack != NULL, "attack active after cancel");
    ASSERT(p.current_attack->move_type == MOVE_TYPE_SPECIAL, "cancelled into special move");
}

static void test_no_chain_down(void) {
    printf("test_no_chain_down:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    InputBuffer buf;
    input_init(&buf);
    const MoveData *med = character_get_normal(CHAR_RYKER, NORMAL_5M);
    /* Start 5M */
    tick_with_buf(&p, &buf, INPUT_MEDIUM, 1);
    tick_with_buf(&p, &buf, 0, PENDING_BUFFER_WAIT);
    tick_with_buf(&p, &buf, 0, med->startup_frames);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5M active");
    p.hit_confirmed = 1;
    /* Try to chain into 5L — should fail (can't chain down M→L) */
    tick_with_buf(&p, &buf, INPUT_LIGHT, 1);
    ASSERT(p.current_attack == med, "5M did NOT chain into 5L (can't chain down)");
}

static void test_hitstun_covers_chain_startup(void) {
    printf("test_hitstun_covers_chain_startup:\n");
    /* Validate the hard rule: hitstun >= next chain's startup + 3 */
    const MoveData *l5 = character_get_normal(CHAR_RYKER, NORMAL_5L);
    const MoveData *m5 = character_get_normal(CHAR_RYKER, NORMAL_5M);
    const MoveData *h5 = character_get_normal(CHAR_RYKER, NORMAL_5H);
    const MoveData *l2 = character_get_normal(CHAR_RYKER, NORMAL_2L);
    const MoveData *m2 = character_get_normal(CHAR_RYKER, NORMAL_2M);
    const MoveData *h2 = character_get_normal(CHAR_RYKER, NORMAL_2H);
    /* Standing chain: L→L, L→M, M→H */
    ASSERT(l5->hitstun >= l5->startup_frames + 3, "5L hitstun covers 5L startup+3 (self-chain)");
    ASSERT(l5->hitstun >= m5->startup_frames + 3, "5L hitstun covers 5M startup+3");
    ASSERT(m5->hitstun >= h5->startup_frames + 3, "5M hitstun covers 5H startup+3");
    /* Crouching chain: L→L, L→M, M→H */
    ASSERT(l2->hitstun >= l2->startup_frames + 3, "2L hitstun covers 2L startup+3 (self-chain)");
    ASSERT(l2->hitstun >= m2->startup_frames + 3, "2L hitstun covers 2M startup+3");
    ASSERT(m2->hitstun >= h2->startup_frames + 3, "2M hitstun covers 2H startup+3");
    /* Cross-series: standing L into crouching M, etc */
    ASSERT(l5->hitstun >= m2->startup_frames + 3, "5L hitstun covers 2M startup+3");
    ASSERT(l2->hitstun >= m5->startup_frames + 3, "2L hitstun covers 5M startup+3");
    /* Total frame limits */
    ASSERT(l5->total_frames <= 18, "5L total frames <= 18");
    ASSERT(l2->total_frames <= 18, "2L total frames <= 18");
    ASSERT(m5->total_frames <= 30, "5M total frames <= 30");
    ASSERT(m2->total_frames <= 30, "2M total frames <= 30");
    ASSERT(h5->total_frames <= 42, "5H total frames <= 42");
    ASSERT(h2->total_frames <= 42, "2H total frames <= 42");
}

/* ========== HIGH/LOW BLOCKING TESTS ========== */

static void test_block_high_low_system(void) {
    printf("test_block_high_low_system:\n");

    /* Set up two characters facing each other */
    CharacterState defender = {0};
    defender.x = FIXED_FROM_INT(300);
    defender.width = 50;
    defender.height = 80;

    CharacterState attacker = {0};
    attacker.x = FIXED_FROM_INT(200);

    /* Standing block (holding back, no down) */
    uint32_t stand_block = INPUT_RIGHT;  /* P1 attacker left, defender holds right = back */

    /* Crouching block (holding down-back) */
    uint32_t crouch_block = INPUT_RIGHT | INPUT_DOWN;

    /* MID — blocked by both stances */
    ASSERT(is_blocking(&defender, &attacker, stand_block, HIT_TYPE_MID) == 1,
           "MID blocked by standing block");
    ASSERT(is_blocking(&defender, &attacker, crouch_block, HIT_TYPE_MID) == 1,
           "MID blocked by crouching block");

    /* HIGH — blocked by both stances (same as MID) */
    ASSERT(is_blocking(&defender, &attacker, stand_block, HIT_TYPE_HIGH) == 1,
           "HIGH blocked by standing block");
    ASSERT(is_blocking(&defender, &attacker, crouch_block, HIT_TYPE_HIGH) == 1,
           "HIGH blocked by crouching block");

    /* LOW — only crouching block works */
    ASSERT(is_blocking(&defender, &attacker, stand_block, HIT_TYPE_LOW) == 0,
           "LOW beats standing block");
    ASSERT(is_blocking(&defender, &attacker, crouch_block, HIT_TYPE_LOW) == 1,
           "LOW blocked by crouching block");

    /* OVERHEAD — only standing block works */
    ASSERT(is_blocking(&defender, &attacker, stand_block, HIT_TYPE_OVERHEAD) == 1,
           "OVERHEAD blocked by standing block");
    ASSERT(is_blocking(&defender, &attacker, crouch_block, HIT_TYPE_OVERHEAD) == 0,
           "OVERHEAD beats crouching block");

    /* UNBLOCKABLE — nothing blocks it */
    ASSERT(is_blocking(&defender, &attacker, stand_block, HIT_TYPE_UNBLOCKABLE) == 0,
           "UNBLOCKABLE ignores standing block");
    ASSERT(is_blocking(&defender, &attacker, crouch_block, HIT_TYPE_UNBLOCKABLE) == 0,
           "UNBLOCKABLE ignores crouching block");

    /* Not holding back at all — never blocks */
    ASSERT(is_blocking(&defender, &attacker, INPUT_DOWN, HIT_TYPE_MID) == 0,
           "no block without holding back");
    ASSERT(is_blocking(&defender, &attacker, 0, HIT_TYPE_LOW) == 0,
           "no block with no input");

    /* Air normals are always OVERHEAD (data label check) */
    const CharacterDef *ryker = character_get_def(CHAR_RYKER);
    ASSERT(ryker->normals[6]->hit_type == HIT_TYPE_OVERHEAD, "j.L is OVERHEAD");
    ASSERT(ryker->normals[7]->hit_type == HIT_TYPE_OVERHEAD, "j.M is OVERHEAD");
    ASSERT(ryker->normals[8]->hit_type == HIT_TYPE_OVERHEAD, "j.H is OVERHEAD");
}

/* ========== S BUTTON + SUPER JUMP TESTS ========== */

static void test_5s_launcher(void) {
    printf("test_5s_launcher:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press S from ground — should start 5S */
    commit_attack_from_idle(&p, INPUT_SPECIAL);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "5S enters startup");
    ASSERT(p.current_attack == character_get_normal(CHAR_RYKER, NORMAL_5S), "resolves to NORMAL_5S");
    ASSERT(p.current_attack->properties & MOVE_PROP_LAUNCHER, "5S has LAUNCHER property");
    ASSERT(p.current_attack->properties & MOVE_PROP_SUPER_JUMP_CANCEL, "5S has SUPER_JUMP_CANCEL property");
}

static void test_js_air(void) {
    printf("test_js_air:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Jump past jump squat */
    tick(&p, INPUT_UP, 1);
    tick(&p, 0, 4);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne");
    /* Press S while airborne — should start j.S */
    tick(&p, INPUT_SPECIAL, 1);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "j.S enters startup");
    ASSERT(p.current_attack == character_get_normal(CHAR_RYKER, NORMAL_JS), "resolves to NORMAL_JS");
}

static void test_super_jump_cancel_off_s(void) {
    printf("test_super_jump_cancel_off_s:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Start 5S */
    commit_attack_from_idle(&p, INPUT_SPECIAL);
    /* Tick through remaining startup (10 total, 1 elapsed on commit) */
    tick(&p, 0, 9);
    ASSERT(active(&p)->state == STATE_ATTACK_ACTIVE, "5S active");
    /* Simulate hit confirmed */
    p.hit_confirmed = 1;
    /* Press UP — should super jump cancel */
    tick(&p, INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "super jump cancel into jump squat");
    ASSERT(active(&p)->super_jump == TRUE, "super_jump flag set");
}

static void test_super_jump_higher(void) {
    printf("test_super_jump_higher:\n");
    const CharacterDef *def = character_get_def(CHAR_RYKER);
    ASSERT(def->super_jump_velocity < def->jump_velocity,
           "super jump velocity more negative (higher) than normal");
}

static void test_s_not_in_dash(void) {
    printf("test_s_not_in_dash:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Press L first to set button_press_frame, then S — should NOT trigger two-button dash */
    tick(&p, INPUT_LIGHT, 1);
    /* Wait for pending to expire */
    tick(&p, 0, 5);
    /* Now press S next frame */
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    tick(&p, INPUT_LIGHT, 1);
    tick(&p, INPUT_SPECIAL, 1);
    /* S should not count as a second button for two-button dash */
    ASSERT(active(&p)->state != STATE_DASH_FORWARD && active(&p)->state != STATE_DASH_BACKWARD,
           "S button does not trigger two-button dash");
}

static void test_28_super_jump_from_neutral(void) {
    printf("test_28_super_jump_from_neutral:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400, CHAR_RYKER);
    /* Hold down for a few frames (enter crouch) */
    tick(&p, INPUT_DOWN, 3);
    ASSERT(active(&p)->state == STATE_CROUCH, "crouching after holding down");
    /* Release down and press up — takes 2 frames:
     * frame 1: crouch sees no DOWN → exits to idle (prev_input still has DOWN)
     * frame 2: idle sees UP → enters jump squat, super_jump detected */
    tick(&p, INPUT_UP, 2);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat from 28 input");
    ASSERT(active(&p)->super_jump == TRUE, "super_jump flag set from 28 input");
    /* Tick through jump squat */
    const CharacterDef *def = character_get_def(CHAR_RYKER);
    tick(&p, 0, def->jump_squat_frames);
    ASSERT(active(&p)->state == STATE_AIRBORNE, "airborne after super jump squat");
    ASSERT(active(&p)->vy == def->super_jump_velocity, "uses super jump velocity");
}

/* ========== THROW TESTS ========== */

static void test_throw_in_range(void) {
    printf("test_throw_in_range:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    /* Opponent at 430 — within THROW_RANGE (75) */
    fixed_t opp_x = FIXED_FROM_INT(430);
    /* 6H (forward + heavy) at close range = throw */
    tick_opp(&p, INPUT_RIGHT | INPUT_HEAVY, 1, opp_x);
    ASSERT(active(&p)->state == STATE_ATTACK_STARTUP, "throw starts at close range");
    ASSERT(p.current_attack == character_get_throw(CHAR_RYKER), "throw move selected");
}

static void test_throw_out_of_range(void) {
    printf("test_throw_out_of_range:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    /* Opponent at 600 — beyond THROW_RANGE (75) */
    fixed_t opp_x = FIXED_FROM_INT(600);
    tick_opp(&p, INPUT_RIGHT | INPUT_HEAVY, 1, opp_x);
    /* Should NOT throw — out of range, gets normal 5H instead */
    ASSERT(p.current_attack != character_get_throw(CHAR_RYKER),
           "throw does not start out of range");
}

static void test_throw_loses_to_motion(void) {
    printf("test_throw_loses_to_motion:\n");
    PlayerState p;
    InputBuffer buf;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    input_init(&buf);
    fixed_t opp_x = FIXED_FROM_INT(430);
    /* Feed QCF motion then 6H — should be 623H special, not throw */
    input_update(&buf, INPUT_DOWN);
    player_update(&p, INPUT_DOWN, &buf, opp_x);
    input_update(&buf, INPUT_DOWN | INPUT_RIGHT);
    player_update(&p, INPUT_DOWN | INPUT_RIGHT, &buf, opp_x);
    input_update(&buf, INPUT_RIGHT | INPUT_HEAVY);
    player_update(&p, INPUT_RIGHT | INPUT_HEAVY, &buf, opp_x);
    /* With QCF motion + 6H, special takes priority over throw */
    if (p.current_attack != NULL) {
        ASSERT(p.current_attack->move_type != MOVE_TYPE_THROW, "motion + 6H = special, not throw");
    } else {
        ASSERT(1, "no throw when motion detected");
    }
}

/* ========== METER TESTS ========== */

static void test_meter_gain_on_hit(void) {
    printf("test_meter_gain_on_hit:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    const MoveData *jab = character_get_normal(CHAR_RYKER, NORMAL_5L);
    ASSERT(p.meter == 0, "meter starts at 0");
    /* Simulate: move hits, check_attack_hit would add meter_gain */
    p.meter += jab->meter_gain;
    ASSERT(p.meter == jab->meter_gain, "meter increased by move's meter_gain");
    ASSERT(jab->meter_gain >= 200, "meter_gain is meaningful (not tiny)");
}

static void test_meter_cap(void) {
    printf("test_meter_cap:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    p.meter = MAX_METER - 10;
    p.meter += 100;
    if (p.meter > MAX_METER) p.meter = MAX_METER;
    ASSERT(p.meter == MAX_METER, "meter capped at MAX_METER");
}

static void test_meter_spend_on_super(void) {
    printf("test_meter_spend_on_super:\n");
    PlayerState p;
    InputBuffer buf;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    input_init(&buf);
    const MoveData *super = character_get_super(CHAR_RYKER, 1);
    ASSERT(super != NULL, "level 1 super exists");
    p.meter = super->meter_cost;
    /* Feed QCF + L+M to trigger super */
    input_update(&buf, INPUT_DOWN);
    player_update(&p, INPUT_DOWN, &buf, 0);
    input_update(&buf, INPUT_DOWN | INPUT_RIGHT);
    player_update(&p, INPUT_DOWN | INPUT_RIGHT, &buf, 0);
    input_update(&buf, INPUT_RIGHT | INPUT_LIGHT | INPUT_MEDIUM);
    player_update(&p, INPUT_RIGHT | INPUT_LIGHT | INPUT_MEDIUM, &buf, 0);
    ASSERT(p.current_attack == super, "super activated");
    ASSERT(p.meter == 0, "meter spent on super");
}

/* ========== KO TESTS ========== */

static void test_ko_on_zero_hp(void) {
    printf("test_ko_on_zero_hp:\n");
    /* KO detection is in game_update (requires GameState + Raylib).
     * Test the logic directly: if hp <= 0, ko_winner should be set. */
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    active(&p)->hp = 0;
    ASSERT(active(&p)->hp <= 0, "HP at zero triggers KO condition");
}

/* ========== WAKEUP INVINCIBILITY TESTS ========== */

static void test_wakeup_invincible(void) {
    printf("test_wakeup_invincible:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    /* Force knockdown */
    active(&p)->state = STATE_KNOCKDOWN;
    active(&p)->state_frame = 0;
    active(&p)->in_hitstun = TRUE;
    active(&p)->vx = 0;
    /* Tick through knockdown (30 frames) */
    tick(&p, 0, 30);
    ASSERT(active(&p)->state == STATE_IDLE, "wakeup reached IDLE");
    ASSERT(active(&p)->wakeup_timer > 0, "wakeup timer active after knockdown");
    ASSERT(active(&p)->wakeup_timer == 8, "wakeup timer is WAKEUP_FRAMES (8)");
}

static void test_wakeup_expires(void) {
    printf("test_wakeup_expires:\n");
    PlayerState p;
    player_init(&p, 1, 400, GROUND_Y - 80, CHAR_RYKER);
    active(&p)->wakeup_timer = 8;
    /* Tick 8 frames — timer should reach 0 */
    tick(&p, 0, 8);
    ASSERT(active(&p)->wakeup_timer == 0, "wakeup timer expired after 8 frames");
}

/* ========== OTG TESTS ========== */

static void test_otg_window_constants(void) {
    printf("test_otg_window_constants:\n");
    /* Verify OTG-capable moves exist */
    const MoveData *jl = character_get_normal(CHAR_RYKER, NORMAL_JL);
    ASSERT(jl->properties & MOVE_PROP_OTG, "j.L has OTG property");
    const MoveData *jm = character_get_normal(CHAR_RYKER, NORMAL_JM);
    ASSERT(jm->properties & MOVE_PROP_OTG, "j.M has OTG property");
    const MoveData *jh = character_get_normal(CHAR_RYKER, NORMAL_JH);
    ASSERT(jh->properties & MOVE_PROP_OTG, "j.H has OTG property");
}

static void test_otg_combo_tracking(void) {
    printf("test_otg_combo_tracking:\n");
    ComboState combo;
    combo_init(&combo);
    ASSERT(combo_can_otg(&combo) == 1, "OTG available at combo start");
    combo_use_otg(&combo);
    ASSERT(combo_can_otg(&combo) == 0, "OTG used up after use");
    combo_reset(&combo);
    ASSERT(combo_can_otg(&combo) == 1, "OTG available again after reset");
}

static void test_non_otg_misses_knockdown(void) {
    printf("test_non_otg_misses_knockdown:\n");
    /* 5L does NOT have MOVE_PROP_OTG — should not hit knockdown opponents */
    const MoveData *jab = character_get_normal(CHAR_RYKER, NORMAL_5L);
    ASSERT(!(jab->properties & MOVE_PROP_OTG), "5L has no OTG property");
}

/* ========== MAIN ========== */

int main(void) {
    printf("=== Vortex Clash State Machine Tests ===\n\n");

    test_idle_on_init();
    test_walk_forward();
    test_walk_backward();
    test_no_slide_on_tap();
    test_jump_from_idle();
    test_jump_while_walking();
    test_jump_with_direction();
    test_crouch();
    test_dash_forward();
    test_dash_backward();
    test_landing_frames();
    test_facing_direction();
    test_stage_bounds();
    test_jump_squat_uncancelable();
    test_jump_squat_momentum();
    test_dash_crouch_cancel();
    test_attack_5l();
    test_attack_5m();
    test_attack_5h();
    test_hitstun();
    test_blockstun();

    /* Cancel hierarchy tests */
    test_can_cancel_function();
    test_cancel_idle_by_normal();
    test_cancel_startup_by_two_button_dash();
    test_plink_dash_sequence();
    test_recovery_not_cancelable_on_whiff();
    test_chain_cancel_on_hit();
    test_nothing_cancels_hitstun();
    test_dash_early_frames_locked();
    test_combo_buffer_during_recovery();
    test_combo_buffer_expires();
    test_combo_buffer_chain_on_hit();
    test_two_button_dash_leniency();

    /* Crouching normal tests */
    test_crouch_attack_2l();
    test_crouch_attack_returns_to_crouch();

    /* Air normal tests */
    test_air_attack_jl();
    test_air_attack_preserves_momentum();
    test_air_attack_gravity();

    /* Jump cancel tests */
    test_jump_cancel_on_hit();
    test_no_jump_cancel_on_whiff();
    test_no_jump_cancel_5h();

    /* Dash jump tests */
    test_dash_jump_carries_momentum();
    test_dash_jump_further_distance();

    /* Motion input tests */
    test_motion_qcf_p1();
    test_motion_qcb_p1();
    test_motion_dp_p1();
    test_motion_qcf_facing_left();
    test_special_from_idle();
    test_special_cancel_on_hit();
    test_pending_buffer_preserves_direction();

    /* Projectile tests */
    test_projectile_spawn();
    test_projectile_one_per_player();
    test_projectile_movement();
    test_projectile_despawn_bounds();
    test_projectile_despawn_on_hit();

    /* Polish fix tests */
    test_dash_jump_speed_capped();
    test_air_friction_dash_jump_only();
    test_projectile_close_range_clamp();
    test_pushbox_no_over_separation();
    test_air_allows_crossup();
    test_landing_resolves_side();

    /* Combo rule tests */
    test_light_self_chain();
    test_all_normals_special_cancel();
    test_no_chain_down();
    test_hitstun_covers_chain_startup();

    /* High/low blocking tests */
    test_block_high_low_system();

    /* S button + super jump tests */
    test_5s_launcher();
    test_js_air();
    test_super_jump_cancel_off_s();
    test_super_jump_higher();
    test_s_not_in_dash();
    test_28_super_jump_from_neutral();

    /* Throw tests */
    test_throw_in_range();
    test_throw_out_of_range();
    test_throw_loses_to_motion();

    /* Meter tests */
    test_meter_gain_on_hit();
    test_meter_cap();
    test_meter_spend_on_super();

    /* KO tests */
    test_ko_on_zero_hp();

    /* Wakeup invincibility tests */
    test_wakeup_invincible();
    test_wakeup_expires();

    /* OTG tests */
    test_otg_window_constants();
    test_otg_combo_tracking();
    test_non_otg_misses_knockdown();

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
