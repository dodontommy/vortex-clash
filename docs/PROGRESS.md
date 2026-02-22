# Progress Tracker

## Current Phase
Phase 7: Sprites, Projectiles, Training Mode & Polish

## Status
IN PROGRESS

## Completed

### Phases 1-5 (Foundation)
- [x] Project setup, build system, Raylib integration
- [x] Fixed-timestep game loop, basic rendering
- [x] Character state machine (idle, walk, crouch, jump, dash, landing)
- [x] Input system with buffering, motion detection, gamepad support
- [x] Hitbox/hurtbox collision, hit resolution, blocking, pushbox

### Phase 6: Combo System & Frame Data
- [x] ComboState with damage scaling and hitstun decay
- [x] Data-driven move definitions (character.h/c, character_data/ryker.c)
- [x] Full Ryker move set: normals (5L/M/H, 2L/M/H, j.L/M/H), specials (236/623/214 L/M/H, j.214 L/M/H), supers (3)
- [x] Chain combos (L→M→H), cancel hierarchy (FREE > NORMAL > SPECIAL > SUPER > NONE)
- [x] Two-button dash, plink dash, combo buffer (5-frame window)
- [x] Jump cancel on hit, crouching normals with return-to-crouch, air normals with gravity
- [x] Dash jump momentum carry
- [x] Facing-relative motion detection, cancel priority chain bug fix
- [x] Special/super anim IDs assigned (ANIM_SPECIAL_BASE, ANIM_SUPER_BASE)

### Phase 6.5: Codebase Audit & Refactoring
- [x] 5 bugs fixed (pushbox, combo, input leak, hitstop global)
- [x] Dead code removed (physics.c/h, unused render/character functions, unused types)
- [x] Duplication eliminated (stage bounds, crouch enter, dash merge)
- [x] Scaffolding: knockdown state, move properties (LAUNCHER/WALL_BOUNCE/GROUND_BOUNCE stubs)

### Phase 7: Sprites, Projectiles, Training Mode (current)
- [x] **Sprite system:** sprite.h/c with texture atlas loading, per-anim frame data from JSON, sprite_draw with facing flip
- [x] **Projectile system:** projectile.h/c — MOVE_PROP_PROJECTILE spawns entity, per-player limit (1), lifetime/bounds despawn, hit detection with block/damage
- [x] **Camera system:** dynamic zoom (1.5x-2.2x) based on player distance, smooth lerp, stage edge clamping
- [x] **Screen shake:** damage-scaled amplitude, decay over frames
- [x] **Visual polish:** hit flash (additive blend), defender vibration during hitstop, special/super move labels
- [x] **Sprite assets:** Ryker sprite sheet (tools/split_spritesheet.py for extraction)
- [x] **is_blocking fix:** now correctly checks hold-back (away from attacker), was checking hold-toward
- [x] **Blockstun pushback:** velocity decays with friction instead of zeroing instantly — blocked attacks have weight
- [x] **Counter-hit system:** hitbox_resolve_hit takes counter_hit param, applies 1.1x damage + 1.2x hitstun
- [x] **P1 gamepad support:** keyboard + gamepad 0 merged for P1 input
- [x] **Training mode:**
  - F2 or gamepad Start toggles menu, game pauses while menu is open
  - Dummy block modes: None, All, After 1st, Crouch, Stand
  - Dummy state modes: Stand, Crouch, Jump
  - Counter-hit toggle, HP reset on combo drop toggle
  - Exit Game option in menu
  - D-pad + A button navigation on controller
  - "TRAINING" indicator on HUD
  - Combo count preserved through blocks in training mode (for After 1st to work)
- [x] **Combo counter HUD:** gold hit count + "HITS" label + damage, fade-out over 1 second after combo drop
- [x] **Wall bounce / ground bounce:** MOVE_PROP_WALL_BOUNCE triggers bounce when defender near wall, MOVE_PROP_GROUND_BOUNCE triggers bounce on landing

## Known Issues / Bugs
- Magic series (chain combos) may break at close range — needs investigation (possibly pushbox/hitbox overlap issue at point-blank)
- Combo counter display may not show in all cases — verify with BLOCK_NONE in training
- No KO system yet — characters at 0 HP continue playing
- `is_blocking` convention: currently checks hold-toward (matches code but unusual for FGs) — FIXED, now checks hold-back

## Phase 7 Remaining (next up)
- [ ] Launcher → air combo transition (super jump cancel on 2H hit)
- [ ] Wakeup invincibility frames
- [ ] OTG window in knockdown state
- [ ] Per-move gatling tables (use cancel_into field)
- [ ] Throw system (L+M proximity check)
- [ ] Super meter gain/spend
- [ ] Debug investigate: magic series close-range failure

## Future Phases
- **Phase 8:** Second character (Titan — grappler), data-driven character loading
- **Phase 9:** Tag team mechanics (raw tag, assists, DHC, snapback, incoming)
- **Phase 10:** Full HUD (meter bars, burst gauge, timer, damage drain animation)
- **Phase 11:** GGPO rollback netcode
- **Phase 12:** Menu system, character select, training mode UI enhancements
- **Phase 13:** Remaining characters (Zara, Viper, Aegis, Blaze) + audio
- **Phase 14:** Polish (sprites, particles, balance, performance)

## Tests
190 tests, all passing

## Blockers
(none)

## Session Log
- Phase 1-5 complete, pushed to GitHub
- Hotfixes: hitbox collision, jump squat momentum, dash crouch cancel
- Full codebase audit & refactoring (Phase 6.5)
- Motion input fix: facing-relative detection, cancel chain priority bug
- Sprite system, projectiles, camera zoom, screen shake, hit flash
- Training mode with full menu (F2/gamepad Start), dummy block/state/counter-hit/HP reset
- Combo counter HUD with fade-out
- P1 gamepad support (keyboard + gamepad 0 merged)
- is_blocking fix: now checks hold-back (away) instead of hold-toward
- Blockstun pushback: friction decay instead of instant zero
- Counter-hit system in hitbox_resolve_hit
- Wall bounce / ground bounce mechanics implemented
