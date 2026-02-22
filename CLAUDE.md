# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Vortex Clash is a 2D tag-team fighting game engine written in C99 using Raylib 5.0. It targets deterministic 60 FPS simulation with fixed-point math, designed for eventual GGPO rollback netcode. Currently in Phase 7 (Sprites, Projectiles, Training Mode).

## Hard Rules (Do Not Break)

- **60 FPS locked.** Never change this. All frame data is measured in 60fps frames.
- **Deterministic simulation.** Use `fixed_t` (16.16 fixed-point) for all physics. No floats in gameplay code.
- **Tests required.** Every new gameplay feature needs headless tests in `tests/test_states.c`.
- **Flat GameState.** No pointers in game state — must be serializable as a flat byte buffer for GGPO.
- **Rendering is separated from simulation.** Camera, animations, effects are render-only. Game logic must not depend on render state.

## Build & Test Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -B build

# Build
cmake --build build

# Run game
./build/vortex_clash

# Run tests (230 headless tests, no Raylib dependency)
ctest --test-dir build
# or directly:
./build/test_states

# Run at increased speed for testing
VORTEX_SPEED=2 ./build/vortex_clash
```

Build uses CMake 3.16+, fetches Raylib 5.0 via FetchContent. Compiler flags: `-Wall -Wextra -Werror`, C99 standard. Tests compile with `TESTING_HEADLESS` define (excludes Raylib).

## Architecture

### Core Loop (`main.c`)
Fixed-timestep accumulator: accumulates real time, runs `game_update()` at exactly 16.67ms intervals, then `game_render()` once per frame. `VORTEX_SPEED` env var multiplies updates per frame (1-4x).

### Game State (`game.h/c`)
`GameState` holds everything: two `PlayerState` (each with two `CharacterState` for tag team), `InputBuffer[2]`, projectiles, hitstop, combo display, training mode, camera. `game_update()` drives the per-frame tick: input polling → player updates → hit checking → projectile updates.

### Player State Machine (`player.h/c`)
States: IDLE, WALK_FORWARD/BACKWARD, CROUCH, JUMP_SQUAT, AIRBORNE, LANDING, DASH_FORWARD/BACKWARD, ATTACK_STARTUP/ACTIVE/RECOVERY, BLOCK_STANDING/CROUCHING, HITSTUN, BLOCKSTUN, KNOCKDOWN. Transitions driven by input + current state in `player_update()`.

### Fixed-Point Math (`types.h`)
All positions, velocities, and physics constants use `fixed_t` (int32_t, 16.16). Key macros: `FIXED_FROM_INT()`, `FIXED_TO_INT()`, `FIXED_MUL()`, `FIXED_DIV()`, `FIXED_ONE`.

### Input System (`input.h/c`)
60-frame ring buffer. Motion detection with leniency windows: QCF (236) = 12 frames, QCB (214) = 12 frames, DP (623) = 15 frames, 360 = 20 frames. Priority: Throw > Burst > Super > Special > Normal. Rising-edge detection via `input_button_pressed()`.

### Character Data (`character.h`, `character_data/*.c`)
Data-driven: `CharacterDef` holds movement stats, dimensions, and arrays of `MoveData` (normals, specials, supers). Each move defines startup/active/recovery frames, damage, hitstun, hitbox dimensions, cancel properties (bitmask: `MOVE_PROP_CHAIN`, `MOVE_PROP_SPECIAL_CANCEL`, etc). Currently only Ryker is implemented.

### Hitbox System (`hitbox.h/c`)
AABB overlap with facing-relative offsets. Hitboxes only exist during ATTACK_ACTIVE. `hitbox_resolve_hit()` handles damage scaling, hitstun decay, counter-hits (1.1x damage, 1.2x hitstun), and state transitions.

### Combo System (`combo.h/c`)
Damage scaling: 100% → 98% → 96% → 92% → ... → 72% (hit 9+). Light starter penalty: 0.8x. Hitstun decay: 100% (hits 1-5) → 85% → 70% → 55% (hits 12+).

### Sprite System (`sprite.h/c`)
Texture atlas loaded from `assets/<char>/sheet.png` + `manifest.txt`. Animations synced to character state via `sprite_sync_anim()`. Asset repacking tool: `tools/repack_sheet.py`.

## Combo & Frame Data Rules (Do Not Break)

These are engine-level guarantees, not per-character tuning knobs.

- **All normals special cancel on hit.** The engine grants at least `CANCEL_BY_SPECIAL` to every normal on hit. `MOVE_PROP_CHAIN` adds normal→normal chains on top. Do NOT rely on `MOVE_PROP_SPECIAL_CANCEL` flags for normals — the engine handles it.
- **Lights self-chain.** Strength-0 normals (L) can chain into themselves (L→L→L). Pushback and hitstun decay are the natural limiters. Mediums and heavies require ascending strength.
- **Hitstun >= next chain's startup + 3.** Every chain route must be a true combo with at least +3 frame advantage. Validate this when adding or modifying moves. If a move chains into something, the hitstun must cover the target's startup frames plus 3.
- **Light normals: total frames ≤ 18.** Jabs must be fast. Mediums ≤ 30, Heavies ≤ 42.
- **Air friction only on dash jumps.** Normal jumps preserve full horizontal momentum. Dash jumps decelerate to prevent absurd distance. Do not apply blanket air friction.

## Blocking & Hit Type Rules (Do Not Break)

- **Move data is authoritative.** `hit_type` on MoveData determines blocking rules. No engine overrides — do NOT add code that silently changes a move's hit_type at runtime.
- **High/low blocking:** `is_blocking()` checks defender stance vs hit_type. Standing block (back) beats HIGH/MID/OVERHEAD. Crouching block (down-back) beats HIGH/MID/LOW. LOW beats standing block. OVERHEAD beats crouching block. UNBLOCKABLE beats everything.
- **Air normals are OVERHEAD in data.** j.L, j.M, j.H should be tagged `HIT_TYPE_OVERHEAD` in character data. This is a data convention, not an engine override.
- **Dive kicks are command normals, not specials.** Input is j.2+button (down+button while airborne). They live in `normals[NORMAL_J2L/J2M/J2H]` slots, NOT in the specials array. They are `MOVE_TYPE_NORMAL` with `HIT_TYPE_MID` — never overhead.
- **`resolve_normal` handles air command normals.** If airborne + holding down, checks j.2+button slots first, falls back to regular j.L/j.M/j.H if slot is empty.
- **Stance requirements are data-driven.** `MoveData.stance` is a `STANCE_*` bitmask controlling where a move can be used. `STANCE_ANY` (0) = anywhere, `STANCE_GROUNDED` = on ground (standing or crouching), `STANCE_AIRBORNE` = air only, `STANCE_STANDING` = not crouching. Specials/supers check `stance_ok()` before executing. All of Ryker's grounded specials and supers use `STANCE_GROUNDED`.
- **Knockdown is rare on normals.** Only sweeps (2H-type moves) should cause knockdown. Normals with knockdown properties (`MOVE_PROP_SLIDING_KD`, `MOVE_PROP_HARD_KD`) should be exceptional. Specials, supers, and throws can knockdown freely.

## Key Design Patterns

- **Bitmask properties:** Move properties use `MOVE_PROP_*` flags combined with `|`.
- **Cancel hierarchy:** FREE > NORMAL > SPECIAL > SUPER > NONE. Moves can cancel into higher-tier moves on hit.
- **Hit IDs:** Each attack gets a unique `hit_id` to prevent multi-hitting. `opponent_hits[]` tracks which characters were already hit.
- **Hitstop:** Global freeze frames on hit for impact feel. Both attacker and defender freeze.

## Testing Pattern

Tests in `test_states.c` are headless (no graphics). Pattern: init game state → set up positions/inputs → call `game_update()` N times → assert state. Tests use `assert()` directly. Each test function is called from `main()`.

## Reference Documents

- `docs/PLAN.md` — Full 85KB design document with game mechanics, character data, and phase roadmap
- `docs/PROGRESS.md` — Current status, completed features, known issues, remaining work
