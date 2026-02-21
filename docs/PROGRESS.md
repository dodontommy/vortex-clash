# Progress Tracker

## Current Phase
Phase 5: Hitbox/Hurtbox & Collision System

## Status
COMPLETE

## Completed
- [x] Create src/hitbox.h / hitbox.c — Hitbox, Hurtbox structs, hitbox_check_collision, hitbox_resolve_hit
- [x] Extend CharacterState with hitstun_remaining, blockstun_remaining, hp (10000 max)
- [x] Add attack states: ATTACK_STARTUP -> ATTACK_ACTIVE -> ATTACK_RECOVERY
- [x] Implement hitstun and blockstun states
- [x] Attack moves: 5L, 5M, 5H defined
- [x] Add hit detection logic to game.c
- [x] Add blocking logic (hold back to block)
- [x] Add health bar rendering
- [x] Add hitstop on hit (8 frames freeze)
- [x] Build compiles, tests pass

## Next Up
- Phase 6: Combo System & Frame Data

## Blockers
(none)

## Hotfixes Applied (by Tommy/Claude Code)
- **BUG FIX**: hitbox_check_collision in game.c was using defender position instead of attacker position — attacks wouldn't connect. Fixed.
- **BUG FIX**: Jump squat now maintains ground momentum — character keeps moving during 4-frame squat instead of freezing in place.
- **FEATURE**: Dash crouch cancel after 6 frames (DASH_CROUCH_CANCEL_FRAMES constant). Both forward and backward dash.
- **TESTS**: Added test_jump_squat_momentum and test_dash_crouch_cancel to test_states.c (55 total tests, all passing).
- **NOTE**: When adding new features, make sure to add corresponding tests. Phase 5 had no combat tests — please add tests for attacks, hitstun, blockstun, blocking in Phase 6.

## Session Log
- Phase 1-5 complete, pushed to GitHub
- Hotfixes applied: hitbox collision bug, jump squat momentum, dash crouch cancel, new tests
