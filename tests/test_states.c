#include <stdio.h>
#include <string.h>
#include "../src/types.h"
#include "../src/player.h"

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
        player_update(p, input);
    }
}

static CharacterState *active(PlayerState *p) {
    return &p->chars[p->active_char];
}

/* ========== TESTS ========== */

static void test_idle_on_init(void) {
    printf("test_idle_on_init:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400);
    ASSERT(active(&p)->state == STATE_IDLE, "starts in IDLE");
    ASSERT(active(&p)->on_ground == TRUE, "starts on ground");
    ASSERT(active(&p)->vx == 0, "no horizontal velocity");
    ASSERT(active(&p)->vy == 0, "no vertical velocity");
}

static void test_walk_forward(void) {
    printf("test_walk_forward:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 400, 400);
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
    player_init(&p, 1, 400, 400);
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
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 200, 400);
    /* Walk right then press up+right simultaneously */
    tick(&p, INPUT_RIGHT, 3);
    ASSERT(active(&p)->state == STATE_WALK_FORWARD, "walking forward");
    tick(&p, INPUT_RIGHT | INPUT_UP, 1);
    ASSERT(active(&p)->state == STATE_JUMP_SQUAT, "jump squat from walk + up");
}

static void test_jump_with_direction(void) {
    printf("test_jump_with_direction:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 400, 400);
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
    player_init(&p, 1, 400, 400);
    /* Double tap left (P1 faces right, so left = backward) */
    tick(&p, INPUT_LEFT, 1);
    tick(&p, 0, 2);
    tick(&p, INPUT_LEFT, 1);
    ASSERT(active(&p)->state == STATE_DASH_BACKWARD, "dash backward on double tap left");
}

static void test_landing_frames(void) {
    printf("test_landing_frames:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400);
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
    player_init(&p1, 1, 200, 400);
    player_init(&p2, 2, 600, 400);
    ASSERT(active(&p1)->facing == 1, "P1 faces right initially");
    ASSERT(active(&p2)->facing == -1, "P2 faces left initially");
    /* Move P1 past P2 */
    for (int i = 0; i < 200; i++) {
        player_update(&p1, INPUT_RIGHT);
        player_update(&p2, 0);
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
    player_init(&p, 1, 10, 400);
    /* Walk left into wall */
    for (int i = 0; i < 100; i++) {
        player_update(&p, INPUT_LEFT);
    }
    ASSERT(active(&p)->x >= 0, "cannot go past left wall");
    /* Walk right into wall */
    player_init(&p, 1, 1200, 400);
    for (int i = 0; i < 100; i++) {
        player_update(&p, INPUT_RIGHT);
    }
    ASSERT(active(&p)->x <= FIXED_FROM_INT(SCREEN_WIDTH - active(&p)->width), "cannot go past right wall");
}

static void test_jump_squat_uncancelable(void) {
    printf("test_jump_squat_uncancelable:\n");
    PlayerState p;
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 200, 400);
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
    player_init(&p, 1, 400, 400);
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

    printf("\n=== Results: %d/%d passed, %d failed ===\n",
           tests_passed, tests_run, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
