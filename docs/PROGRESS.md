# Progress Tracker

## Current Phase
Phase 9: Tag Team Mechanics

## Status
Phase 8 COMPLETE — Phase 9 IN PROGRESS

## Phase 9 Tasks
- [ ] Extend PlayerState to hold 2 CharacterState structs (point + assist), implement active_character switching
- [ ] Raw Tag: hold A1 15+ frames to tag out. Departing character jumps out (vulnerable ~20 frames). Incoming character enters with landing animation (10 frames, invincible frames 1-5). Cannot tag during blockstun/hitstun or assist cooldown
- [ ] Assist Call: tap A1 (<15 frames) to call off-screen partner. Assist performs predetermined attack then exits. Cooldown 180 frames (300 if assist was hit). Assist takes 150% damage while on screen
- [ ] DHC (Delayed Hyper Combo): during super active frames, input partner's super + A1 to cancel into partner's super. Costs 1 additional bar
- [ ] Snapback: QCF+A1 (1 bar). 20f startup, mid hit, -5 on block. On hit forces opponent's point out and drags assist in
- [ ] Incoming Mixup: when character KO'd or snapped, incoming enters from top of screen. Player can hold direction to influence trajectory. 5 invincible frames on landing
- [ ] Blue Health: portion of damage becomes recoverable on off-screen partner. Recovers at 5 HP/frame while tagged out, paused during incoming
- [ ] Team KO: match ends when both characters on one team reach 0 HP
- [ ] Tests for all new mechanics (tag switching, assist calls, DHC, snapback, blue health recovery)

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

### Phase 7: Sprites, Projectiles, Training Mode & Gameplay Systems
- [x] **Sprite system:** sprite.h/c with texture atlas loading, per-anim frame data from JSON, sprite_draw with facing flip
- [x] **Projectile system:** projectile.h/c — MOVE_PROP_PROJECTILE spawns entity, per-player limit (1), lifetime/bounds despawn, hit detection with block/damage
- [x] **Camera system:** dynamic zoom (1.5x-2.2x) based on player distance, smooth lerp, stage edge clamping
- [x] **Screen shake:** damage-scaled amplitude, decay over frames
- [x] **Visual polish:** hit flash (alpha-tinted overlay), defender vibration during hitstop, special/super move labels
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
- [x] **Throw system:** 6H/4H proximity throw input, grounded-only checks, deterministic throw lock/thrown sequence, character_get_throw() API
- [x] **Super meter:** Gain on hit (attacker full, defender half), MAX_METER=5000, meter bar rendering under health bars, super spend on activation
- [x] **KO system:** HP<=0 triggers KO freeze (120 frames), "K.O.!" text centered on screen, auto-reset both players after timer
- [x] **Wakeup invincibility:** 8 frames of invincibility after knockdown ends, attacks during wakeup frames are skipped
- [x] **OTG window:** Knockdown frames 5-20 are hittable by MOVE_PROP_OTG moves only, one OTG per combo (combo_can_otg/combo_use_otg)

### Phase 7.5: Gameplay Polish Pass
- [x] **Dash jump speed cap:** Horizontal air speed from dash jumps capped to 8 px/frame (was 20). Dash jumps still go farther than normal jumps but no longer cross the entire stage.
- [x] **Air friction: dash jumps only.** Normal jumps preserve full horizontal momentum. Only dash jumps decelerate (~0.125 px/frame²). This is an engine rule, not a tuning knob.
- [x] **Projectile close-range fix:** Fireballs no longer spawn behind the opponent at point-blank range. Spawn position clamped so leading edge doesn't pass defender's front edge.
- [x] **Pushbox separation fix:** Changed from `overlap/2 + 1` to `(overlap+1)/2` ceiling division — eliminates over-push jitter at close range.
- [x] **Air-to-ground pushbox:** Airborne players are now pushed to one side of grounded opponents, preventing full horizontal overlap. Deterministic side resolution based on center positions — crossups are now clear and readable.
- [x] **Engine-level cancel rules (not per-move flags):**
  - All normals on hit grant at least CANCEL_BY_SPECIAL (special + super cancel). MOVE_PROP_CHAIN adds normal→normal chains on top.
  - Lights (strength 0) can self-chain: L→L→L. Pushback and hitstun decay are the natural limiters.
  - Mediums/heavies require strictly ascending strength for chains.
- [x] **Frame data audit:** All chain routes now satisfy hitstun >= next startup + 3. Jab total frames reduced to ≤18. Standing and crouching magic series are true combos.

### Phase 8: S Button, Super Jump, Launcher, Key Remapping
- [x] S button: 5S (universal launcher) and j.S (air exchange/spikedown)
- [x] Super jump cancel off 5S on hit (MOVE_PROP_SUPER_JUMP_CANCEL)
- [x] 28 super jump from neutral (down→up input)
- [x] Key remapping sub-menu in training mode

## Known Issues / Bugs
- Combo counter display may not show in all cases — verify with BLOCK_NONE in training
- `is_blocking` convention: currently checks hold-toward (matches code but unusual for FGs) — FIXED, now checks hold-back

## Future Phases
- **Phase 9:** Tag team mechanics (raw tag, assists, DHC, snapback, incoming)
- **Phase 10:** Full HUD (burst gauge, timer, damage drain animation)
- **Phase 11:** Second character (Titan — grappler), data-driven character loading
- **Phase 12:** GGPO rollback netcode
- **Phase 13:** Menu system, character select, training mode UI enhancements
- **Phase 14:** Remaining characters (Zara, Viper, Aegis, Blaze) + audio
- **Phase 15:** Polish (sprites, particles, balance, performance)

## Tests
384 tests, all passing

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
- Phase 8: S button, super jump, launcher, key remapping
- Phase 7 completion: throws, meter, KO, wakeup invincibility, OTG window (384 tests passing)
