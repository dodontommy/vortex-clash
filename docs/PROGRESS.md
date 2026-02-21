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

## Session Log
- Phase 1-5 complete, pushed to GitHub
