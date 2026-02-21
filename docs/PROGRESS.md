# Progress Tracker

## Current Phase
Phase 5: Hitbox/Hurtbox & Collision System

## Status
IN PROGRESS

## Completed
- [x] Create src/hitbox.h / hitbox.c — Hitbox, Hurtbox structs, hitbox_check_collision, hitbox_resolve_hit
- [x] Extend CharacterState with hitstun_remaining, blockstun_remaining, hp (10000 max)
- [x] Add attack states: ATTACK_STARTUP -> ATTACK_ACTIVE -> ATTACK_RECOVERY
- [x] Implement hitstun and blockstun states
- [x] Attack moves: 5L, 5M, 5H defined
- [x] Build compiles, tests pass

## Next Up
- [ ] Add hit detection logic to game.c
- [ ] Add blocking logic (hold back to block)
- [ ] Add health bar rendering
- [ ] Add hitstop on hit
- [ ] Verify all acceptance criteria

## Blockers
(none)

## Session Log
- Phase 1-4 complete
- Phase 5 in progress
