# VORTEX CLASH -- 2D Tag-Team Fighting Game

## Technical Plan & Game Design Document

**Engine:** Raylib (C99) | **Netcode:** GGPO Rollback | **Target:** 60 FPS Deterministic Simulation

---

## Table of Contents

1. [Game Design Overview](#1-game-design-overview)
2. [Tag Team Mechanics](#2-tag-team-mechanics)
3. [Combo System](#3-combo-system)
4. [Universal Mechanics](#4-universal-mechanics)
5. [Character Archetypes & Roster](#5-character-archetypes--roster)
6. [Input System](#6-input-system)
7. [Technical Architecture](#7-technical-architecture)
8. [Development Phases](#8-development-phases)

---

## 1. Game Design Overview

### Core Identity

A 2v2 tag-team fighting game blending the chaotic assist-driven neutral of Marvel vs Capcom with the system-rich universal mechanics of Dragon Ball FighterZ. The game prioritizes aggressive play, long creative combos, and constant tag-team synergy.

### Match Format

- **Team Size:** 2v2 (point character + assist character)
- **Round Structure:** No rounds. Each team has two health bars (one per character). The team whose characters both reach zero HP first loses.
- **Timer:** 99 seconds per match (99 * 60 = 5940 game frames). When time runs out, the team with higher total remaining HP wins. If equal, draw.
- **Screen:** 1280x720 base resolution, 16:9 aspect ratio. Stage width is approximately 1600 logical pixels (camera scrolls).

### Health & Damage Scaling

- Each character has **10000 HP**.
- Damage scales down as combos extend (see Combo System section).
- Recoverable (blue) health: when a character is hit, a portion of the damage dealt becomes "blue health" on the off-screen partner. Blue health recovers slowly while the partner is tagged out (recovery rate: 5 HP/frame while off-screen, paused during incoming mixup).

---

## 2. Tag Team Mechanics

### 2.1 Tag / Raw Tag

- **Input:** Hold Assist button (A1) for 15+ frames.
- **Behavior:** Current point character jumps off-screen (airborne, vulnerable for ~20 frames during the jump arc). The assist character enters from behind the point character's position with a short landing animation (10 frames, invincible frames 1-5).
- **Restrictions:** Cannot raw tag during blockstun, hitstun, or while the assist character is on cooldown (see 2.2). Can tag during your own attack recovery or neutral states.
- **On hit during tag-out jump:** The departing character is snapped back in (combo continues on them). This punishes careless tags.

### 2.2 Assist Calls

- **Input:** Press Assist button (A1) briefly (< 15 frames).
- **Behavior:** The off-screen partner jumps in, performs a single predetermined assist attack, then exits.
- **Cooldown:** 180 frames (3 seconds) after the assist character fully exits screen. Cooldown displayed on HUD.
- **Vulnerability:** The assist character has a hurtbox during the entire assist animation. If hit, they take 150% damage (assist punish). Hitting the assist also forces a longer cooldown (300 frames / 5 seconds).
- **Assist Types:** Each character has one assist move (hardcoded per character for the initial build).

### 2.3 Delayed Hyper Combo (DHC)

- **Input:** During a super move's active frames, input the partner's super move command + Assist button.
- **Behavior:** The current character's super is canceled. They exit; the partner enters and immediately performs their super.
- **Cost:** 1 additional super meter bar (the partner's super cost). Total = point super cost + partner super cost.
- **Purpose:** Extends combos, swaps characters safely, maximizes damage.

### 2.4 Snapback

- **Input:** QCF + Assist button (costs 1 meter bar).
- **Behavior:** A universal attack (20 frame startup, mid hit, -5 on block). If it connects, the opponent's point character is forced off-screen and their assist character is dragged in.
- **Purpose:** Force in a low-health character, remove a problematic point character, reset blue health recovery.

### 2.5 Incoming Mixup

When a character is KO'd or snapped out, the incoming character enters from a screen-edge position at the top. The incoming player can hold a direction to influence entry trajectory (left, right, or neutral). The entering character has 5 frames of invincibility upon landing. The attacker can attempt to mix up the incoming character during this transition.

---

## 3. Combo System

### 3.1 Chain Combo System (Magic Series)

Normals cancel into each other in a fixed ascending order:

```
Light (L) -> Medium (M) -> Heavy (H) -> Special -> Super
```

- **Ground chain:** L -> L -> M -> M -> H (maximum 2 of each Light/Medium, 1 Heavy)
- **Air chain:** j.L -> j.M -> j.H (one of each in the air)
- **Launcher:** Crouching H (2H) is a universal launcher that sends the opponent into the air. On hit, the attacker can super jump to pursue.

### 3.2 Link System

Beyond chains, certain moves have enough frame advantage on hit to "link" into normals that would not normally chain. This rewards precise timing and character knowledge.

### 3.3 Launchers & Air Combos

- **Launcher (2H):** Crouching Heavy. On hit, pops the opponent upward. The attacker automatically super jumps if they hold up after a successful launcher.
- **Super Jump:** Achieved by pressing down then up quickly (2 -> 8, within 4 frames) or automatically after launcher. Grants higher jump arc (1.5x normal jump height). Air normals only chain once per super jump.
- **Air combo limit:** Enforced by "hitstun decay." Each successive hit in an air combo reduces the hitstun the opponent suffers. After approximately 10-12 air hits, the opponent can tech out (recover in the air).

### 3.4 Wall Bounce

- Certain moves (character-specific, typically a special move) cause wall bounce on hit.
- **Limit:** 1 wall bounce per combo. A second wall-bounce move in the same combo will cause a hard knockdown instead.
- The opponent bounces off the wall and travels back toward center stage, remaining in a hitstun state for a brief window to allow follow-up.

### 3.5 Ground Bounce (OTG)

- Certain moves can hit grounded (knocked down) opponents -- these are "OTG-capable" moves.
- **Limit:** 1 OTG pickup per combo. A second OTG attempt causes the opponent to tech roll automatically.
- OTG moves typically have limited hitstun, forcing the attacker to act quickly.

### 3.6 Sliding Knockdown & Hard Knockdown

- **Sliding Knockdown:** The opponent slides along the ground and can be OTG'd. Caused by specific enders (e.g., air H at the end of an air combo sends them to the ground sliding).
- **Hard Knockdown:** The opponent is stuck on the ground for a fixed duration (30 frames) and cannot tech. Caused by specific supers or the second wall-bounce/ground-bounce in a combo.

### 3.7 Damage Scaling

Combo damage is scaled based on:

| Hit Number | Damage Multiplier |
|------------|-------------------|
| 1          | 100%              |
| 2          | 98%               |
| 3          | 96%               |
| 4          | 92%               |
| 5          | 88%               |
| 6          | 84%               |
| 7          | 80%               |
| 8          | 76%               |
| 9+         | 72% (minimum)     |

- Light starters further reduce scaling: if the combo begins with a Light normal, multiply all damage by an additional 0.8x.
- Supers have minimum damage: supers always deal at least 30% of their base damage regardless of scaling.

### 3.8 Hitstun Decay

Each hit in a combo reduces the hitstun of subsequent hits:

- Hits 1-5: 100% hitstun
- Hits 6-8: 85% hitstun
- Hits 9-11: 70% hitstun
- Hits 12+: 55% hitstun (effectively uncomboable for most moves)

When hitstun frames reach 0, the opponent can act (they are in a "floating" recovery state and can air tech).

---

## 4. Universal Mechanics

### 4.1 Blocking

- **Stand Block:** Hold back (4) to block standing attacks and air attacks. Does NOT block lows.
- **Crouch Block:** Hold down-back (1) to block low attacks and standing attacks. Does NOT block overheads.
- **Air Block:** Hold back while airborne to block. Cannot air block during the first 4 frames of a non-super jump. Cannot air block during super jump at all (rewards anti-airing with launcher).
- **Blockstun:** Each move applies a fixed blockstun value. The defender cannot act during blockstun but can input pushblock.
- **Chip Damage:** Special moves and supers deal 10% of their base damage as chip (block) damage. Chip damage cannot KO (leaves opponent at 1 HP minimum). Normals deal 0 chip.
- **Crossup Protection:** For the first 4 frames of blockstun, blocking direction is locked (prevents instant crossup unreactables from assists).

### 4.2 Pushblock (Advancing Guard)

- **Input:** Press L + M while in blockstun.
- **Behavior:** The defender creates a shockwave that pushes the attacker back significantly (200 pixels). Adds 5 frames of blockstun to the defender, but the gap created makes it very difficult for the attacker to continue pressure.
- **Uses:** Escape pressure, create space, push out assist characters.

### 4.3 Burst (Combo Breaker)

- **Input:** Press L + M + H + A1 simultaneously (all four buttons).
- **Resource:** Requires a full "Burst Gauge" that starts full at the beginning of a match and refills over 20 real seconds (1200 frames) of not being in hitstun/blockstun.
- **Behavior:** An invincible frame-1 shockwave that knocks the opponent away. Can be used in hitstun or blockstun to escape combos or pressure. Very short range (approximately 80 pixels around the character).
- **Risk:** If the opponent baits the burst (by stopping their combo early and blocking), the burst has 40 frames of recovery and the attacker gets a full punish. Burst gauge does not refill for 30 seconds (1800 frames) if whiffed.
- **Visual:** Large flash effect and a unique audio cue so both players clearly recognize it.

### 4.4 Super Meter

- **Meter Bars:** 0 to 5 bars (each bar = 1000 meter points).
- **Meter Gain:**
  - Attacking (on hit): Each hit grants `base_damage * 0.05` meter to the attacker.
  - Attacking (on block): Each hit grants `base_damage * 0.025` meter to the attacker.
  - Being hit: Defender gains `damage_dealt * 0.10` meter.
  - Special moves (whiff): Grants 20 meter on whiff.
- **Meter Usage:**
  - Level 1 Super: 1 bar (1000 meter).
  - Level 3 Super: 3 bars (3000 meter).
  - DHC: +1 bar on top of the incoming super's cost.
  - Snapback: 1 bar.
  - EX Specials: 0.5 bar (500 meter). Enhanced version of a special move with better properties.
- **Meter carries over** between character KOs within the same match.

### 4.5 Sparking Blast (Vortex Trigger)

- **Input:** Press L + M + H + A1 while NOT in hitstun/blockstun (same button combination as Burst, but context-dependent: neutral = Sparking, hitstun/blockstun = Burst).
- **Resource:** Can be used once per match per team.
- **Duration:** Depends on how many characters are alive:
  - 2 characters alive: 600 frames (10 seconds)
  - 1 character alive: 900 frames (15 seconds)
- **Effects:**
  - Chip damage from normals (1% of base damage).
  - Blue health regeneration speed tripled (15 HP/frame).
  - All normals are jump-cancelable on block and on hit.
  - +10% damage boost to all attacks.
  - Can be activated mid-combo to extend combos (similar to Roman Cancel in Guilty Gear).
- **Visual:** Character glows with energy particles. HUD shows a draining timer bar.

---

## 5. Character Archetypes & Roster

Starting roster of 6 characters. Each has a unique playstyle and represents a core fighting game archetype.

### 5.1 Character Template (Shared Normals Layout)

Every character has the following moves:

**Ground Normals:**
| Input   | Name              | Properties |
|---------|-------------------|------------|
| 5L      | Standing Light    | Fast, short range, chains into itself and 5M |
| 5M      | Standing Medium   | Mid-range poke, chains into 5H |
| 5H      | Standing Heavy    | Slow, long range, causes wall bounce on some characters |
| 2L      | Crouching Light   | Low, fast, chains into 2M |
| 2M      | Crouching Medium  | Low, mid-range |
| 2H      | Crouching Heavy   | Universal launcher. Pops opponent into air |
| 6M      | Forward Medium    | Overhead. Slow startup (24 frames), hits mid-high. Does not chain. |

**Air Normals:**
| Input   | Name              | Properties |
|---------|-------------------|------------|
| j.L     | Jumping Light     | Fast air poke |
| j.M     | Jumping Medium    | Good air-to-air |
| j.H     | Jumping Heavy     | Strong downward hitbox, jump-in tool |

**Throws:**
| Input   | Name              | Properties |
|---------|-------------------|------------|
| 5L+M (close) | Forward Throw | 5 frame startup, 130 pixel range, unblockable, techable with same input within 7 frames |
| 4L+M (close) | Back Throw    | Same as forward throw but switches sides |

**Universal:**
| Input   | Name              | Properties |
|---------|-------------------|------------|
| 2H      | Launcher          | Chains from normals, super jump cancelable on hit |
| QCF+A1  | Snapback          | Costs 1 bar. Forces character switch on hit. |

### 5.2 Roster

---

#### Character 1: RYKER (Rushdown / Shoto)

**Theme:** Balanced martial artist. Good at everything, great at nothing. Entry point character.

**Specials:**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Fireball        | 14/16/18| 3      | 28       | Projectile, L=slow, M=mid, H=fast. EX: 2 hits. |
| 623L/M/H    | Rising Upper    | 5/7/9   | 8/10/12| 30       | Invincible frames 1-5 (L). Anti-air. EX: full invincibility. |
| 214L/M/H    | Slide Kick      | 12/14/16| 4      | 22       | Low. L=short, H=long. EX: causes sliding knockdown. |
| j.214L/M/H  | Divekick        | 10      | until land | 12   | Alters air trajectory. EX: bounces on hit. |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M      | Hadou Barrage   | 1 bar| 11+3    | 2500   | Fast beam super. Full-screen. |
| 214L+M      | Rush Combo      | 1 bar| 8+2     | 2800   | Cinematic combo super. Close range. |
| 236H+S      | Ultimate Blast  | 3 bar| 5+5     | 4500   | Invincible startup. Full-screen beam. |

**Assist:** 236M Fireball (mid-speed projectile, covers approach)

---

#### Character 2: ZARA (Zoner / Trap)

**Theme:** Ranged specialist who controls space with projectiles and traps.

**Specials:**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Energy Orb      | 18/20/22| --     | 24       | Projectile: L=straight, M=45-degree up, H=45-degree down. Slow-moving. EX: orb stays on screen for 120 frames as a trap. |
| 214L/M/H    | Tripwire        | 20      | 300    | 30       | Places a ground trap at varying distances. Detonates on contact. Max 2 traps. EX: trap is invisible. |
| 623L/M/H    | Teleport        | 18/15/12| --     | 14       | L=behind opponent, M=above opponent, H=original position. No invincibility. EX: 5 frames invincible during teleport. |
| j.236L/M/H  | Air Orb         | 16      | --     | 20       | Air projectile. L=straight, M=45-down, H=straight-down. |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M      | Orb Barrage     | 1 bar| 15+5    | 2400   | Fires 8 orbs in a spread pattern. |
| 214L+M      | Minefield       | 1 bar| 10+0    | 2200   | Places 5 traps across the screen instantly. Last 300 frames. |
| 236H+S      | Satellite Laser | 3 bar| 10+8    | 4200   | Full-screen vertical beam that sweeps horizontally. |

**Assist:** 236L Energy Orb (slow projectile that covers neutral space)

---

#### Character 3: TITAN (Grappler / Tank)

**Theme:** Huge, slow, powerful. Relies on hard reads, command grabs, and armor.

**Specials:**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 360L/M/H    | Cyclone Grab    | 2/2/2   | 3      | 45       | Command grab. L=800dmg, M=1000dmg, H=1200dmg. EX: 1500dmg + wall bounce. Cannot be comboed into. |
| 623L/M/H    | Shoulder Charge | 15/18/21| 6      | 28       | Armor (absorbs 1 hit) during frames 5-15. Causes wall bounce. EX: absorbs 3 hits. |
| 214L/M/H    | Ground Stomp    | 20      | 4      | 32       | Hits low. Causes ground bounce. L=close, H=far. EX: full screen earthquake. |
| j.214L/M/H  | Body Splash     | 14      | until land | 20   | Armor during active frames. Hard knockdown on hit. |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M      | Titan Crusher   | 1 bar| 5+1     | 3000   | Command grab super. 2-frame post-flash. |
| 214L+M      | Earthquake      | 1 bar| 18+4    | 2600   | Full screen low. Causes hard knockdown. |
| 236H+S      | Apocalypse Drop | 3 bar| 8+1     | 5000   | Cinematic command grab. Highest damage in game. |

**Unique Properties:**
- 12000 HP (20% more than standard).
- Walk speed 70% of normal.
- Jump is lower and slower.
- Takes 90% damage from all sources (innate 10% damage reduction).

**Assist:** 623M Shoulder Charge (armored approach assist, great for covering rushdown)

---

#### Character 4: VIPER (Rushdown / Mixup)

**Theme:** Lightning-fast high/low/left/right mixup character. Glass cannon.

**Specials:**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Shadow Dash     | 8/10/12 | --     | 16       | Dash forward. L=short, M=mid, H=goes through opponent. EX: fully invincible. |
| 214L/M/H    | Low Slide       | 10/12/14| 5      | 18       | Low-hitting slide. L=short, H=full screen. EX: causes hard knockdown. |
| 623L/M/H    | Flash Kick      | 4/6/8   | 10     | 32       | Invincible reversal (L only: invincible frames 1-6). Anti-air. EX: wall bounces. |
| j.236L/M/H  | Air Dash Attack | 8       | 6      | 14       | Air dash with an attack. L=forward, M=down-forward, H=straight down. |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M      | Shadow Onslaught| 1 bar| 7+2     | 2600   | Fast dash super. Mid-range. |
| 214L+M      | Phantom Dive    | 1 bar| 5+1     | 2400   | Air-OK. Diagonal dive attack. Overhead. |
| 236H+S      | Viper's Fang    | 3 bar| 3+2     | 4800   | Close range cinematic. Fastest super startup. |

**Unique Properties:**
- 9000 HP (10% less than standard).
- Walk speed 130% of normal.
- Double airdash (can airdash twice before landing).

**Assist:** 236M Shadow Dash (fast dash-in attack, locks down opponent)

---

#### Character 5: AEGIS (Defensive / Counter)

**Theme:** Defensive specialist with counters, barriers, and punish tools.

**Specials:**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Barrier         | 10      | 60/90/120 | 20    | Creates a stationary barrier that blocks 1/2/3 projectiles. EX: reflects projectiles. |
| 214L/M/H    | Counter Stance  | 5       | 25/30/35| 20     | Parry pose. If struck by a mid or high during active frames, auto-counters with a launcher (2H equivalent). EX: also catches lows. |
| 623L/M/H    | Shield Bash     | 12/14/16| 5      | 22       | Forward-advancing attack. Guard point frames 1-8 (auto-blocks one hit then continues attack). EX: causes crumple stun. |
| j.214L/M/H  | Falling Shield  | 12      | until land | 16   | Downward attack with guard point. |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M      | Fortress        | 1 bar| 8+0     | 0      | Installs a damage-absorbing shield (absorbs up to 1500 damage) for 300 frames. |
| 214L+M      | Grand Counter   | 1 bar| 1       | 3200   | Frame-1 counter super. Only activates on hit. Whiff = 60 frame recovery. |
| 236H+S      | Aegis Reflector | 3 bar| 5+3     | 4000   | Creates a massive reflector that hits the opponent 6 times. Can be placed at varying distances. |

**Assist:** 236M Barrier (places a barrier that absorbs a projectile, strong defensive assist)

---

#### Character 6: BLAZE (Stance / Versatile)

**Theme:** Stance-switching character with two distinct modes (Fire and Ice), rewarding adaptability.

**Stance Mechanic:** Press 22+S (down-down+Special) to toggle between Fire Mode and Ice Mode. Stance switch takes 20 frames (vulnerable). Each mode changes special move properties.

**Specials (Fire Mode):**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Fire Wave       | 12/14/16| 4      | 24       | Fast projectile, 1 hit. EX: multi-hit (3). |
| 623L/M/H    | Flame Uppercut  | 6/8/10  | 9      | 28       | Anti-air, invincible (L: frames 1-4). EX: fully invincible. |
| 214L/M/H    | Blaze Rush      | 10/12/14| 6      | 20       | Forward dash attack. Overhead on H version. |

**Specials (Ice Mode):**
| Input       | Name            | Startup | Active | Recovery | Properties |
|-------------|-----------------|---------|--------|----------|------------|
| 236L/M/H    | Ice Shard       | 16/18/20| --     | 26       | Slow projectile that "freezes" (adds 8 extra hitstun frames). EX: creates a lingering icicle trap. |
| 623L/M/H    | Frost Spike     | 8/10/12 | 5      | 30       | Anti-air pillar from the ground. EX: reaches full screen vertically. |
| 214L/M/H    | Glacier Slide   | 14/16/18| 6      | 22       | Low slide. Causes sliding knockdown. EX: freezes on hit (extra 15 hitstun). |

**Supers:**
| Input       | Name            | Cost | Startup | Damage | Properties |
|-------------|-----------------|------|---------|--------|------------|
| 236L+M (Fire) | Inferno Cannon| 1 bar| 10+4    | 2600   | Full-screen horizontal fire beam. |
| 236L+M (Ice)  | Absolute Zero | 1 bar| 12+6    | 2200   | Full-screen ice wave. Freezes for 45 extra hitstun frames. |
| 236H+S      | Elemental Storm | 3 bar| 8+5     | 4600   | Mode-independent. Fire+Ice combined cinematic super. |

**Assist (Fire):** 236M Fire Wave (fast projectile)
**Assist (Ice):** 236M Ice Shard (slow projectile with freeze property)

---

## 6. Input System

### 6.1 Notation

Standard numpad notation (based on a number pad, assuming Player 1 faces right):

```
7 8 9       (up-back, up, up-forward)
4 5 6       (back, neutral, forward)
1 2 3       (down-back, down, down-forward)
```

**Buttons:**
- `L` = Light Attack
- `M` = Medium Attack
- `H` = Heavy Attack
- `A1` = Assist / Tag
- Combinations: `L+M` = Throw / Pushblock, `L+M+H+A1` = Burst / Sparking

### 6.2 Motion Inputs

| Notation | Name              | Input Sequence | Detection Window |
|----------|-------------------|----------------|------------------|
| 236      | Quarter Circle Forward (QCF) | Down, Down-Forward, Forward | 12 frames |
| 214      | Quarter Circle Back (QCB)    | Down, Down-Back, Back | 12 frames |
| 623      | Dragon Punch (DP)            | Forward, Down, Down-Forward | 15 frames |
| 360      | Full Circle (for grabs)      | Any 4 cardinal + 2 diagonals in sequence | 20 frames |
| 22       | Double Tap Down              | Down, Neutral, Down | 15 frames |

### 6.3 Input Buffer

- **Buffer Window:** 8 frames. If a button press occurs within 8 frames before a character becomes actionable (e.g., during recovery, blockstun, hitstun), the input is stored and executed on the first actionable frame.
- **Motion Buffer:** Motion inputs (directional sequences) are stored for 12 frames. The final button press must occur within the buffer window.
- **Negative Edge:** NOT implemented (button release does not trigger attacks). This keeps things simpler.
- **Priority System (Simultaneous Inputs):**
  1. Throw (L+M) > any single button.
  2. Burst/Sparking (L+M+H+A1) > everything.
  3. Supers (motion + 2 buttons) > Specials (motion + 1 button) > Normals.
  4. H > M > L (when multiple single normals pressed on same frame).

### 6.4 Input Handling Implementation

```
struct InputBuffer {
    uint32_t raw_inputs[INPUT_BUFFER_SIZE];  // Ring buffer of raw input states
    int write_index;                          // Current write position
    int frames_stored;                        // How many frames are stored
};

// Each frame, the raw input is a bitmask:
#define INPUT_UP        (1 << 0)
#define INPUT_DOWN      (1 << 1)
#define INPUT_LEFT      (1 << 2)
#define INPUT_RIGHT     (1 << 3)
#define INPUT_L         (1 << 4)
#define INPUT_M         (1 << 5)
#define INPUT_H         (1 << 6)
#define INPUT_A1        (1 << 7)

// Derived:
#define INPUT_THROW     (INPUT_L | INPUT_M)
#define INPUT_BURST     (INPUT_L | INPUT_M | INPUT_H | INPUT_A1)
```

The input parser scans the buffer each frame to detect:
1. Newly pressed buttons (rising edge detection: `current & ~previous`).
2. Motion sequences by scanning backwards through the directional history.
3. Combined inputs by checking simultaneous button presses.

---

## 7. Technical Architecture

### 7.1 Project Structure

```
vortex_clash/
|-- CMakeLists.txt
|-- README.md
|-- PLAN.md
|-- src/
|   |-- main.c                  # Entry point, top-level game loop
|   |-- game.h / game.c         # Core game state, initialization, update, render
|   |-- input.h / input.c       # Input reading, buffering, motion detection
|   |-- player.h / player.c     # Player struct, character state machine
|   |-- character.h / character.c # Character data definitions (frame data, moves)
|   |-- character_data/          # Per-character data files
|   |   |-- ryker.c
|   |   |-- zara.c
|   |   |-- titan.c
|   |   |-- viper.c
|   |   |-- aegis.c
|   |   |-- blaze.c
|   |-- hitbox.h / hitbox.c     # Hitbox/hurtbox definitions, collision detection
|   |-- combo.h / combo.c       # Combo counter, damage scaling, hitstun decay
|   |-- physics.h / physics.c   # Position, velocity, gravity, stage boundaries
|   |-- animation.h / animation.c # Sprite animation system
|   |-- hud.h / hud.c           # Health bars, meter, timer, combo counter display
|   |-- menu.h / menu.c         # Main menu, character select, pause menu
|   |-- audio.h / audio.c       # Sound effects and music management
|   |-- network.h / network.c   # GGPO integration, session management
|   |-- rollback.h / rollback.c # Save/load game state for rollback
|   |-- render.h / render.c     # Rendering helpers, camera, screen shake
|   |-- types.h                 # Common type definitions, constants
|-- assets/
|   |-- sprites/                # Character sprite sheets (PNG)
|   |-- audio/
|   |   |-- sfx/                # Sound effects (WAV)
|   |   |-- music/              # Background music (OGG)
|   |-- fonts/                  # UI fonts
|   |-- ui/                     # HUD elements, menu backgrounds
|-- lib/
|   |-- ggpo/                   # GGPO SDK headers and library
|-- tools/
|   |-- hitbox_editor/          # (Optional) Simple hitbox visualization tool
|-- build/
```

### 7.2 Game Loop (Fixed Timestep, 60 FPS)

```c
// Pseudocode
#define TICKS_PER_SECOND 60
#define FIXED_DT (1.0 / TICKS_PER_SECOND)  // ~16.67ms

void game_loop(void) {
    double accumulator = 0.0;
    double previous_time = get_time();

    while (!should_quit) {
        double current_time = get_time();
        double frame_time = current_time - previous_time;
        previous_time = current_time;

        // Clamp to avoid spiral of death
        if (frame_time > 0.25) frame_time = 0.25;
        accumulator += frame_time;

        // Poll inputs once per render frame
        poll_inputs();

        while (accumulator >= FIXED_DT) {
            // In netplay: this calls ggpo_advance_frame() instead
            game_update(FIXED_DT);
            accumulator -= FIXED_DT;
        }

        // Interpolation factor for smooth rendering (optional)
        double alpha = accumulator / FIXED_DT;
        game_render(alpha);
    }
}
```

**Critical:** The `game_update()` function must be **fully deterministic**. Given the same inputs and same starting state, it must produce the exact same resulting state. This means:
- No floating-point math in gameplay simulation. Use **fixed-point integers** or carefully controlled integer math.
- No reliance on pointers, memory addresses, or non-deterministic system calls.
- All random elements (if any) must use a seeded PRNG stored in game state.

### 7.3 Fixed-Point Math

To ensure determinism across different machines:

```c
// 16.16 fixed-point
typedef int32_t fixed_t;

#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)   // 65536
#define INT_TO_FP(x) ((fixed_t)((x) << FP_SHIFT))
#define FP_TO_INT(x) ((int)((x) >> FP_SHIFT))
#define FP_MUL(a, b) ((fixed_t)(((int64_t)(a) * (int64_t)(b)) >> FP_SHIFT))
#define FP_DIV(a, b) ((fixed_t)(((int64_t)(a) << FP_SHIFT) / (b)))
```

All positions, velocities, and accelerations use `fixed_t`. Rendering converts to screen pixels via `FP_TO_INT()`.

### 7.4 Game State Structure

```c
#define MAX_PLAYERS 2
#define MAX_CHARS_PER_TEAM 2
#define MAX_PROJECTILES 16
#define MAX_HITBOXES_PER_FRAME 8

typedef struct {
    // Per-character state
    CharacterState characters[MAX_PLAYERS][MAX_CHARS_PER_TEAM];

    // Per-player state
    PlayerState players[MAX_PLAYERS];

    // Global match state
    int timer_frames;        // Countdown timer
    int hitstop_frames;      // Global hitstop (freeze frames on hit)
    int slow_motion_frames;  // For dramatic supers
    int screen_shake_frames;
    int screen_shake_intensity;
    uint32_t rng_state;      // Deterministic PRNG

    // Projectiles
    Projectile projectiles[MAX_PROJECTILES];
    int projectile_count;

    // Camera
    fixed_t camera_x;
    fixed_t camera_y;

    // Round state
    int match_state;  // MATCH_INTRO, MATCH_FIGHT, MATCH_KO, MATCH_RESULT
} GameState;

typedef struct {
    int active_character;   // 0 or 1 (index into team)
    int meter;              // 0-5000 (each 1000 = 1 bar)
    int burst_gauge;        // 0-1000 (1000 = full)
    int sparking_available; // 1 = available, 0 = used
    int sparking_timer;     // Remaining sparking frames (0 = inactive)
    int assist_cooldown;    // Frames until assist is available
    InputBuffer input_buffer;
} PlayerState;

typedef struct {
    int char_id;              // Which character (RYKER, ZARA, etc.)
    int state;                // Current state machine state
    int state_frame;          // How many frames in current state
    fixed_t pos_x, pos_y;    // Position (fixed-point)
    fixed_t vel_x, vel_y;    // Velocity (fixed-point)
    int facing;               // 1 = right, -1 = left
    int hp;                   // Current health
    int blue_hp;              // Recoverable health (on the other character)
    int hitstun_remaining;
    int blockstun_remaining;
    int is_airborne;
    int air_actions_remaining; // For double jumps, air dashes
    int combo_hits;            // Current combo hit counter (0 = not in combo)
    int combo_damage;          // Total combo damage dealt
    int otg_used;              // Has OTG been used this combo?
    int wall_bounce_used;      // Has wall bounce been used this combo?
    int ground_bounce_used;    // Has ground bounce been used this combo?
    int is_point;              // 1 = on screen, 0 = off screen (assist)

    // Animation state
    int anim_id;
    int anim_frame;

    // Active hitboxes/hurtboxes for current frame
    Hitbox hitboxes[MAX_HITBOXES_PER_FRAME];
    int hitbox_count;
    Hurtbox hurtboxes[MAX_HITBOXES_PER_FRAME];
    int hurtbox_count;
} CharacterState;
```

### 7.5 Character State Machine

Each character runs a finite state machine. States and their transitions:

```
IDLE
  -> WALK_FORWARD        (hold 6)
  -> WALK_BACKWARD       (hold 4)
  -> CROUCH              (hold 2)
  -> JUMP_SQUAT          (press 7/8/9)
  -> ATTACK_STARTUP      (press L/M/H)
  -> BLOCK_STANDING      (hold 4 when attacked)
  -> BLOCK_CROUCHING     (hold 1 when attacked)
  -> HITSTUN_STANDING    (hit by attack while standing)
  -> THROW_STARTUP       (press L+M near opponent)
  -> SPECIAL_STARTUP     (valid motion + button)
  -> SUPER_STARTUP       (valid motion + 2 buttons + enough meter)
  -> DASH_FORWARD        (tap 6,6)
  -> DASH_BACKWARD       (tap 4,4)

WALK_FORWARD / WALK_BACKWARD
  -> IDLE                (release direction)
  -> Any state reachable from IDLE

CROUCH
  -> IDLE                (release down)
  -> CROUCH_BLOCK        (hold 1 when attacked)
  -> ATTACK_STARTUP      (press L/M/H while crouching -> crouching normals)

JUMP_SQUAT (4 frames, cannot be canceled)
  -> AIRBORNE

AIRBORNE
  -> AIR_ATTACK_STARTUP  (press L/M/H)
  -> AIR_BLOCK           (hold 4, if allowed)
  -> AIR_DASH            (tap 6,6 or 4,4 in air)
  -> LANDING             (reach ground)
  -> AIR_HITSTUN         (hit by attack while airborne)

ATTACK_STARTUP
  -> ATTACK_ACTIVE       (startup frames complete)

ATTACK_ACTIVE
  -> ATTACK_RECOVERY     (active frames complete)
  -> ATTACK_STARTUP      (chain cancel into next normal on hit/block)
  -> SPECIAL_STARTUP     (special cancel on hit/block)
  -> SUPER_STARTUP       (super cancel on hit/block)
  -> JUMP_SQUAT          (jump cancel, if allowed by Sparking or specific moves)

ATTACK_RECOVERY
  -> IDLE                (recovery frames complete)
  -> (very limited cancels allowed here, mainly supers)

HITSTUN_STANDING / HITSTUN_CROUCHING / HITSTUN_AIR
  -> IDLE / AIRBORNE     (hitstun expires)
  -> BURST               (if burst available, input burst command)
  -> HITSTUN_LAUNCHED    (hit by launcher during ground hitstun)
  -> KNOCKDOWN           (specific knockdown moves)

HITSTUN_LAUNCHED
  -> HITSTUN_AIR         (ascending -> descending)

BLOCKSTUN
  -> IDLE                (blockstun expires)
  -> PUSHBLOCK           (input L+M during blockstun)
  -> BURST               (if available)

KNOCKDOWN
  -> WAKEUP              (after knockdown duration, play wakeup animation)
  -> OTG_HITSTUN         (hit by OTG-capable move)

WAKEUP (invincible for variable frames depending on tech option)
  -> IDLE

THROW_STARTUP (5 frames)
  -> THROW_WHIFF         (no opponent in range -> recovery)
  -> THROW_CONNECT       (opponent grabbed -> cinematic)

DEAD
  -> (character is KO'd, trigger incoming sequence for partner)
```

### 7.6 Hitbox / Hurtbox System

```c
typedef struct {
    int x_offset;     // Relative to character position (fixed-point)
    int y_offset;
    int width;
    int height;
    int damage;
    int chip_damage;
    int hitstun;       // Frames of hitstun on hit
    int blockstun;     // Frames of blockstun on block
    int hit_type;      // HIT_HIGH, HIT_MID, HIT_LOW, HIT_UNBLOCKABLE
    int knockback_x;   // Horizontal knockback velocity (fixed-point)
    int knockback_y;   // Vertical knockback velocity (fixed-point, negative = up)
    int properties;    // Bitmask: WALL_BOUNCE, GROUND_BOUNCE, OTG, LAUNCHER, KNOCKDOWN, etc.
    int meter_gain;    // Meter gained by attacker on hit
    int hit_id;        // Unique ID to prevent multi-hitting (unless intended)
} Hitbox;

typedef struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
    int is_invincible;   // If true, ignore hitbox collisions
    int is_armor;        // If true, take hit but don't enter hitstun
    int armor_hits;      // Number of hits armor can absorb
} Hurtbox;
```

**Collision Detection (AABB):**

```c
bool check_collision(Hitbox *atk, int atk_x, int atk_y, int atk_facing,
                     Hurtbox *def, int def_x, int def_y) {
    // Flip x_offset based on facing direction
    int ax = atk_x + (atk->x_offset * atk_facing);
    int ay = atk_y + atk->y_offset;
    int dx = def_x + def->x_offset;  // Hurtboxes don't flip (symmetric)
    int dy = def_y + def->y_offset;

    // AABB overlap test
    return (ax < dx + def->width &&
            ax + atk->width > dx &&
            ay < dy + def->height &&
            ay + atk->height > dy);
}
```

**Collision Resolution Priority:**
1. Check all attacker hitboxes vs all defender hurtboxes.
2. If multiple hitboxes collide, use the one with the highest damage.
3. Mutual hits (both players hit each other on same frame): both take the hit (trade).
4. Hitbox vs hitbox collision: projectiles can cancel each other. Normal attack hitboxes do NOT interact with each other (both can hit simultaneously).

### 7.7 Frame Data System

Each move is defined as a sequence of frames:

```c
typedef struct {
    int frame;                 // Frame number within the move (0-indexed)
    Hitbox hitboxes[4];        // Active hitboxes on this frame (max 4)
    int hitbox_count;
    Hurtbox hurtboxes[4];      // Hurtbox adjustments on this frame
    int hurtbox_count;
    int sprite_index;          // Which sprite to display
    fixed_t move_x;            // Forced horizontal movement this frame
    fixed_t move_y;            // Forced vertical movement this frame
    int cancellable;           // Bitmask: CAN_CHAIN, CAN_SPECIAL, CAN_SUPER, CAN_JUMP
    int flags;                 // Bitmask: INVINCIBLE, ARMOR, PROJECTILE_INVULN, etc.
} MoveFrame;

typedef struct {
    const char *name;
    int total_frames;          // Total duration
    int startup_frames;        // Frames before first active frame
    int active_frames;         // Frames with active hitbox
    int recovery_frames;       // Frames after active, before actionable
    MoveFrame *frames;         // Array of per-frame data
    int on_hit_advantage;      // Frame advantage on hit (can be derived)
    int on_block_advantage;    // Frame advantage on block (can be derived)
    int move_type;             // NORMAL, SPECIAL, SUPER, THROW
    int input_command;         // Which input triggers this move
} MoveData;
```

### 7.8 GGPO Rollback Netcode Integration

GGPO requires the game to implement these callbacks:

```c
// Save the entire game state to a buffer
bool save_game_state(unsigned char **buffer, int *len, int *checksum, int frame);

// Load a game state from a buffer (rollback)
bool load_game_state(unsigned char *buffer, int len);

// Free a previously saved game state buffer
void free_buffer(void *buffer);

// Called to advance the game by one frame
bool advance_frame(int flags);

// Called when GGPO detects a desync
bool on_event(GGPOEvent *info);
```

**Key Implementation Details:**

- `save_game_state`: Serializes the entire `GameState` struct to a flat byte buffer. This must capture EVERY piece of game-relevant state. Use `memcpy` on the struct (ensure no pointers in the struct -- all data must be inline or indexed).
- `load_game_state`: Deserializes the byte buffer back into `GameState`. After this call, the game must be in the exact state it was when saved.
- `advance_frame`: Reads inputs from GGPO, calls `game_update()`, increments frame counter.
- The game state struct must be **pointer-free** and **flat** for fast serialization. Use arrays with fixed sizes, indices instead of pointers.

**GGPO Session Setup:**

```c
GGPOSession *ggpo;
GGPOSessionCallbacks callbacks = {
    .save_game_state = save_game_state,
    .load_game_state = load_game_state,
    .free_buffer = free_buffer,
    .advance_frame = advance_frame,
    .on_event = on_event,
};

// For online play
ggpo_start_session(&ggpo, &callbacks, "vortex_clash", 2, sizeof(uint32_t), local_port);
ggpo_set_disconnect_timeout(ggpo, 3000);
ggpo_set_disconnect_notify_start(ggpo, 1000);

// Add players
GGPOPlayer p1 = { .type = GGPO_PLAYERTYPE_LOCAL, .player_num = 1 };
GGPOPlayer p2 = { .type = GGPO_PLAYERTYPE_REMOTE, .player_num = 2, .u.remote = { remote_ip, remote_port } };
ggpo_add_player(ggpo, &p1, &p1_handle);
ggpo_add_player(ggpo, &p2, &p2_handle);
```

**Per frame (online):**

```c
// Synchronize inputs
uint32_t local_input = read_local_input();
ggpo_add_local_input(ggpo, local_handle, &local_input, sizeof(uint32_t));

// GGPO calls advance_frame() as many times as needed (potentially multiple for rollback)
ggpo_idle(ggpo, time_since_last);
```

### 7.9 Camera System

```c
// Camera tracks the midpoint between both point characters
fixed_t target_x = (p1_pos_x + p2_pos_x) / 2;
// Clamp to stage boundaries
// Smooth interpolation (for rendering only; the logical camera snap is deterministic)
camera_x = target_x;  // For determinism, no smoothing in simulation
```

The camera defines the visible area. The stage is wider than the screen. Characters cannot move past the camera edges (push box system).

### 7.10 Push Box System

Each character has an invisible "push box" (AABB). When two push boxes overlap:
- The characters are pushed apart equally (each moves half the overlap distance).
- This prevents characters from occupying the same space.
- Push boxes also interact with stage edges: characters cannot be pushed off-screen.

```c
typedef struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
} PushBox;
```

### 7.11 Rendering Architecture

Rendering is **completely separate** from simulation. The render step reads `GameState` and draws:

1. **Background layer** (parallax scrolling stage art).
2. **Characters** (sprite based on `anim_id` and `anim_frame`, flipped based on `facing`).
3. **Projectiles**.
4. **Hit effects / particles** (sparks, flashes).
5. **HUD overlay** (health bars, meter, timer, combo counter).
6. **Debug overlay** (optional: hitbox/hurtbox visualization, frame data display).

Sprites are loaded as texture atlases. Each character has a sprite sheet with all animation frames packed. An accompanying data file maps `(anim_id, anim_frame)` to `(atlas_x, atlas_y, width, height)`.

### 7.12 Audio Architecture

```c
typedef struct {
    Sound sfx[SFX_COUNT];    // Loaded sound effects
    Music bgm_tracks[BGM_COUNT]; // Background music tracks
    int current_bgm;
    float sfx_volume;
    float bgm_volume;
} AudioSystem;
```

**Sound effects are NOT part of rollback state.** Audio is fire-and-forget from the render/presentation layer. During rollback, audio is suppressed (GGPO provides a flag for this). Only play audio on "confirmed" frames to avoid stuttering.

---

## 8. Development Phases

Each phase includes: **objective, deliverables, acceptance criteria**, and **key implementation notes**.

---

### Phase 1: Project Setup & Build System

**Objective:** Create the project skeleton, configure CMake, integrate Raylib, and verify the build compiles and runs on the target platform.

**Deliverables:**
- `CMakeLists.txt` with Raylib dependency (fetched via CMake FetchContent or as a submodule).
- Directory structure as defined in Section 7.1 (all folders created, placeholder `.c`/`.h` files with header guards).
- `src/main.c` that opens a 1280x720 Raylib window, displays "Vortex Clash" text, and exits on ESC.
- `src/types.h` with all common type definitions: `fixed_t`, fixed-point macros, input bitmask defines, enums for `CharacterId`, `GameMatchState`, `CharacterStateEnum`, `HitType`, `MoveType`.

**Acceptance Criteria:**
- [ ] `cmake --build build` succeeds with no errors or warnings on the target platform.
- [ ] Running the executable opens a 1280x720 window titled "Vortex Clash".
- [ ] The window displays the game title as text.
- [ ] Pressing ESC or closing the window terminates the program cleanly.
- [ ] `types.h` compiles and all macros (`INT_TO_FP`, `FP_TO_INT`, `FP_MUL`, `FP_DIV`) produce correct results (verified by a simple test in `main.c` or a separate test file).

**Key Notes:**
- Use CMake `FetchContent` to pull Raylib from its GitHub release. Pin to a specific version (e.g., 5.0).
- Set C standard to C99 or C11.
- Enable `-Wall -Wextra -Werror` (GCC/Clang) or `/W4` (MSVC).
- Create a `.gitignore` that excludes `build/`, IDE files, and OS files.

---

### Phase 2: Game Loop & Basic Rendering

**Objective:** Implement the fixed-timestep game loop, basic rendering pipeline, and display a placeholder character sprite (a colored rectangle) that can be moved with arrow keys.

**Deliverables:**
- `src/game.h / game.c`: `GameState` struct (simplified: just one character's position for now), `game_init()`, `game_update()`, `game_render()`.
- `src/physics.h / physics.c`: Position and velocity types using `fixed_t`. Gravity constant. `physics_update()` that applies velocity and gravity to a character's position.
- `src/render.h / render.c`: `render_character()` draws a colored rectangle at the character's position. `render_stage()` draws a simple flat ground plane and background color.
- Fixed-timestep loop in `main.c` as described in Section 7.2.
- Camera system: camera X tracks the character (or midpoint if two rectangles are present).

**Acceptance Criteria:**
- [ ] Game runs at exactly 60 logical updates per second regardless of render framerate.
- [ ] A colored rectangle (representing a character) is rendered on screen, standing on a ground plane.
- [ ] Pressing left/right moves the character. Pressing up makes the character jump (with gravity bringing them back down).
- [ ] The character cannot fall through the ground (ground collision at `y = 0` in game coordinates).
- [ ] The character cannot walk off the edges of the stage (stage boundary collision).
- [ ] All movement uses fixed-point math (`fixed_t`). No `float` or `double` in `game_update()` or `physics_update()`.
- [ ] Frame counter is visible on screen (debug text showing current game frame).

**Key Notes:**
- Ground plane Y = 0 in game coordinates. Screen Y is inverted (Y increases downward in Raylib), so convert: `screen_y = SCREEN_HEIGHT - FP_TO_INT(game_y)`.
- Gravity: approximately `INT_TO_FP(1)` per frame downward (tunable). Jump velocity: approximately `INT_TO_FP(-18)` (upward).
- Stage boundaries: `x_min = INT_TO_FP(-800)`, `x_max = INT_TO_FP(800)` (total 1600 logical pixels wide).
- Walk speed: approximately `INT_TO_FP(4)` per frame.

---

### Phase 3: Character State Machine

**Objective:** Implement the full character state machine with all core states and transitions. Replace direct movement with state-driven behavior.

**Deliverables:**
- `src/player.h / player.c`:
  - `CharacterState` struct with state enum, state frame counter, position, velocity, facing direction, flags.
  - `PlayerState` struct wrapping a `CharacterState` for the point character.
  - `player_update(PlayerState *player, uint32_t input)` -- the main per-frame update that runs the state machine.
  - State functions: `state_idle()`, `state_walk_forward()`, `state_walk_backward()`, `state_crouch()`, `state_jump_squat()`, `state_airborne()`, `state_landing()`, `state_dash_forward()`, `state_dash_backward()`.
  - Facing direction auto-correction: characters always face each other (recalculated when both are actionable).

**Acceptance Criteria:**
- [ ] Character visually idles when no input is held (idle state, distinct from walking).
- [ ] Walk forward/backward at different speeds (forward is faster: `INT_TO_FP(4)`, backward: `INT_TO_FP(3)`).
- [ ] Crouch: hold down to enter crouch state. Character rectangle shrinks vertically (or changes color to indicate crouch).
- [ ] Jump: pressing up enters 4-frame jump squat (character is stuck), then transitions to airborne with upward velocity. Gravity pulls them down. Landing triggers a 2-frame landing animation.
- [ ] Directional jump: pressing up-forward or up-backward during jump squat sets horizontal velocity accordingly.
- [ ] Forward dash: tap forward twice quickly (within 10 frames). Character dashes forward for 18 frames at 1.5x walk speed. Cannot act during dash.
- [ ] Backward dash: tap backward twice quickly. Character dashes backward for 22 frames. Airborne frames 4-18 (can be hit as airborne). Invincible frames 1-5.
- [ ] Two rectangles (Player 1 and Player 2) on screen simultaneously, each controlled by different keys (P1: WASD + JKL, P2: Arrow keys + Numpad 123). Both run independent state machines.
- [ ] Characters always face each other. Crossing over an opponent flips both characters' facing directions.
- [ ] State name displayed above each character as debug text.

**Key Notes:**
- Implement as a `switch` on the state enum inside `player_update()`. Each state case checks input and timer to determine transitions.
- Jump squat is uncancelable (no action during those 4 frames). This is critical for jump startup being punishable.
- Backward dash invincibility is tracked via a flag on `CharacterState`.

---

### Phase 4: Input System with Buffering

**Objective:** Implement a proper fighting game input system with buffering, motion detection, and button-press tracking.

**Deliverables:**
- `src/input.h / input.c`:
  - `InputBuffer` struct: ring buffer of `uint32_t` raw inputs, stores last 60 frames of input.
  - `input_update(InputBuffer *buf, uint32_t raw)`: pushes new input into the buffer.
  - `input_button_pressed(InputBuffer *buf, uint32_t button)`: returns true if `button` was pressed this frame (rising edge).
  - `input_button_held(InputBuffer *buf, uint32_t button)`: returns true if button is held this frame.
  - `input_detect_motion(InputBuffer *buf, MotionType motion)`: scans the directional history to detect QCF (236), QCB (214), DP (623), double-tap (66/44), HCF (41236), HCB (63214), and 360.
  - `input_get_buffered_action(InputBuffer *buf, CharacterState *state)`: returns the highest-priority action that should be executed, considering the 8-frame buffer window and the character's current state (what cancels are available).
  - Button mapping layer: maps keyboard keys (or gamepad buttons) to the abstract `INPUT_*` bitmask. Configurable.
  - Gamepad support via Raylib's gamepad API.

**Acceptance Criteria:**
- [ ] Pressing and releasing a button triggers `input_button_pressed()` only on the first frame.
- [ ] Motion detection correctly identifies QCF (236) when down, down-forward, forward is input within 12 frames, regardless of extra inputs between them (leniency).
- [ ] Motion detection correctly identifies DP (623) input: forward, down, down-forward within 15 frames.
- [ ] Input buffer: if a button is pressed during the last 8 frames of a state's recovery, the action is executed on the first actionable frame.
- [ ] Double-tap detection (66 and 44) works reliably and does not trigger on slow directional inputs.
- [ ] Simultaneous button detection: pressing L+M on the same frame is recognized as a throw input.
- [ ] Priority system: when QCF+L+M is pressed, it registers as a super (QCF+LM) rather than a special (QCF+L) plus a throw (L+M).
- [ ] Both keyboard and gamepad inputs work through the same abstraction layer.
- [ ] Debug overlay showing: current input bitmask, detected motions, buffer state.

**Key Notes:**
- For motion detection, scan backwards through the buffer. For QCF, look for a `2` input, then a `3` input after it, then a `6` input after that, all within the window.
- Allow "shortcut" inputs: pressing `3` (down-forward) counts as both `2` (down) and `6` (forward) for the purpose of motion detection. This means `3, 3, 6` is a valid QCF (the common "sloppy" input).
- For DP, `6, 2, 3` is the strict input but also accept `6, 3, 6` and `6, 2, 3, 6` (common leniency).
- 360 motion: detect at least 4 distinct cardinal/diagonal directions in a sequential rotation within 20 frames. Can start from any direction.

---

### Phase 5: Hitbox/Hurtbox & Collision System

**Objective:** Implement the hitbox/hurtbox system, AABB collision detection, hit resolution, and basic attack states so characters can punch each other.

**Deliverables:**
- `src/hitbox.h / hitbox.c`:
  - `Hitbox` and `Hurtbox` structs as defined in Section 7.6.
  - `hitbox_check_collision()`: AABB overlap test between a hitbox and hurtbox, accounting for character position and facing direction.
  - `hitbox_resolve_hit()`: Given a collision, apply damage, hitstun, knockback, and transition the defender to the appropriate hitstun state.
  - Hit tracking: each hitbox has a `hit_id`. A hitbox can only hit a given character once per active period (prevents multi-hitting on single-hit moves).
- Extend `CharacterState` with: `hitstun_remaining`, `blockstun_remaining`, `hp`.
- Extend state machine with attack states:
  - `ATTACK_STARTUP` -> `ATTACK_ACTIVE` -> `ATTACK_RECOVERY`
  - Pressing L while idle starts a `5L` attack (e.g., 5 frames startup, 3 frames active, 10 frames recovery).
  - During active frames, a hitbox is present relative to the character.
- Hitstun and blockstun states:
  - `HITSTUN_STANDING`: character recoils, cannot act for `hitstun_remaining` frames.
  - `BLOCKSTUN`: character is pushed back, cannot act for `blockstun_remaining` frames.
- Blocking: if the defender holds back (relative to attacker's position) when a hitbox makes contact, they block instead of getting hit. Apply blockstun instead of hitstun and chip damage instead of full damage.
- Pushbox collision: characters cannot overlap. When pushboxes collide, push both characters apart.

**Acceptance Criteria:**
- [ ] Pressing L makes the character perform a standing light attack (visible as a hitbox rectangle extending from the character for the active frames). Use debug rendering: green rectangles for hurtboxes, red for hitboxes.
- [ ] When Player 1's hitbox overlaps Player 2's hurtbox, Player 2 enters hitstun (changes color, recoils).
- [ ] Player 2's HP decreases by the attack's damage value.
- [ ] If Player 2 holds back while being attacked, they block: reduced pushback, blockstun instead of hitstun, chip damage only for specials (0 for normals).
- [ ] Hitstun prevents all actions for its duration. After hitstun, the character returns to idle.
- [ ] Hit ID system: a 5L with 3 active frames hits the opponent at most once, even if the hitbox overlaps on all 3 active frames.
- [ ] Pushbox prevents characters from walking through each other.
- [ ] Attacks cannot hit behind the character (hitbox is only in front based on facing).
- [ ] Health bars displayed above each character (simple colored rectangles for now).

**Key Notes:**
- Implement hit/block flash: on hit, freeze both characters for 8 frames (hitstop). This is essential fighting game game-feel. Hitstop is not the same as hitstun -- it's a brief freeze on BOTH characters to emphasize the hit.
- During hitstop, the attacker can buffer their next input (this enables the feel of "confirm on hit").
- Knockback: on hit, apply horizontal velocity to the defender (pushes them away). On block, apply reduced knockback.

---

### Phase 6: Combo System & Frame Data

**Objective:** Implement the full normal attack set, chain combo system, special moves, launcher, air combos, damage scaling, and hitstun decay. This is the most complex gameplay phase.

**Deliverables:**
- `src/character.h / character.c`: Data-driven move definitions. Each character has an array of `MoveData` structs defining all their normals and specials.
- `src/character_data/ryker.c`: Complete move set for Ryker (the first character) with full frame data as specified in Section 5.2.
- `src/combo.h / combo.c`:
  - `ComboState` struct: hit counter, damage accumulator, scaling multiplier, hitstun decay level, OTG/wall-bounce/ground-bounce flags.
  - `combo_on_hit()`: called when a hit connects. Increments counter, applies scaling, returns actual damage dealt.
  - `combo_reset()`: called when the opponent returns to neutral (combo dropped).
  - Damage scaling table as defined in Section 3.7.
  - Hitstun decay as defined in Section 3.8.

- **Chain combo system:** In `ATTACK_ACTIVE` and `ATTACK_RECOVERY`, if the attack hit or was blocked, allow canceling into the next normal in the chain (L->M->H) or into specials/supers.
- **Launcher (2H):** On hit, pops opponent into the air. Attacker can super jump cancel.
- **Air combo:** After launcher, attacker pursues with air chain: j.L -> j.M -> j.H.
- **Special moves:** Implement all of Ryker's specials (236, 623, 214, j.214) with L/M/H variants.
- **Super moves:** Implement Ryker's Level 1 and Level 3 supers. Require meter (hardcode starting meter for testing).
- **EX Specials:** L+M version of specials costs 500 meter, has enhanced properties.
- **Wall bounce, ground bounce, OTG:** Implement the mechanics and per-combo limits as described in Section 3.4-3.5.
- **Knockdown and wakeup:** Implement sliding knockdown, hard knockdown, tech rolls, and wakeup states.

**Acceptance Criteria:**
- [ ] Ryker can perform the full ground chain: 5L -> 5L -> 5M -> 5M -> 5H. Each attack visually executes and has correct frame data.
- [ ] Chain cancel timing is correct: the next normal comes out only if the button is pressed during the previous attack's active or recovery frames AND the previous attack made contact.
- [ ] 2H (launcher) pops the opponent into the air on hit. Holding up after a successful 2H causes a super jump.
- [ ] Air combo: j.L -> j.M -> j.H works. After j.H, the opponent is sent to the ground (sliding knockdown).
- [ ] Full BnB combo route works: 5L, 5M, 5H, 2H, super jump, j.L, j.M, j.H. Combo counter displays correctly.
- [ ] Damage scaling is applied: later hits in the combo deal progressively less damage as per the scaling table.
- [ ] Hitstun decay: after ~12 hits, the opponent can air tech out of the combo.
- [ ] Special moves (236L/M/H fireball, 623L/M/H uppercut, 214L/M/H slide) all function with correct motion inputs.
- [ ] Fireball: a projectile spawns and travels across the screen. It has a hitbox. It can be blocked. It disappears on hit or when off-screen.
- [ ] Rising Upper (623L) is invincible on startup and functions as a reversal (can be performed during wakeup or out of blockstun gaps).
- [ ] Supers consume meter. Level 1 costs 1 bar, Level 3 costs 3 bars. If meter is insufficient, the super does not come out.
- [ ] Combo counter displayed: shows hit count and total damage.
- [ ] Wall bounce triggers on Ryker's 5H (or specified move). Only once per combo; the second occurrence causes hard knockdown instead.

**Key Notes:**
- This phase is large. Implement normals first, then chain cancels, then launcher/air combos, then specials, then supers. Test each subsystem before moving to the next.
- Projectiles need their own struct and update loop. They are part of `GameState` and are deterministic.
- Super moves can use a "super flash" -- a brief pause (20-30 frames) where the screen darkens and only the super user can act. This is visual but has gameplay implications (the defender can react during superflash to block).

---

### Phase 7: Second Character & Character Data Architecture

**Objective:** Implement a second playable character (Titan -- the grappler) and refactor the character system to be fully data-driven so adding more characters is straightforward.

**Deliverables:**
- Refactor `character.c` so that all character-specific data (normals frame data, special move definitions, movement stats, hitbox sizes) is loaded from per-character data files.
- `src/character_data/titan.c`: Complete move set for Titan as defined in Section 5.2, including:
  - Unique movement stats (slower walk, lower jump, higher HP).
  - Command grab (360 motion).
  - Armor on shoulder charge.
  - Ground stomp (low, causes ground bounce).
  - All supers including the command grab super.
- Character select: at game start, each player can press a key to choose between Ryker and Titan (simple text-based selection for now).
- Ensure the state machine handles character-specific properties (Titan's armor, command grabs) generically.

**Acceptance Criteria:**
- [ ] Titan is playable and feels distinctly different from Ryker: slower, more powerful, with armored moves and command grabs.
- [ ] 360 motion input is detected correctly and the command grab executes. It does not combo from normals (startup is too fast to chain into, but the grab has minimum range requirements).
- [ ] Titan's shoulder charge absorbs one hit (armor) and continues attacking.
- [ ] Titan has 12000 HP and takes 90% damage.
- [ ] Character data is defined in separate files. Adding a new character requires creating a new data file and registering it -- no changes to core engine code.
- [ ] Player can choose Ryker or Titan before the match starts.
- [ ] Both characters can fight each other with all mechanics working (blocking, combos, specials, supers).

**Key Notes:**
- Command grabs are unblockable but have 0 range (opponent must be within grab range). They also have a whiff animation if no opponent is in range.
- Armor is implemented in the hurtbox: when `is_armor` is true and `armor_hits > 0`, the character takes damage but does not enter hitstun. Decrement `armor_hits`.

---

### Phase 8: Tag Team Mechanics

**Objective:** Implement the full 2v2 tag-team system: team selection, raw tag, assists, DHC, snapback, incoming mixup, and blue health.

**Deliverables:**
- Extend `PlayerState` to hold 2 `CharacterState` structs (point and assist).
- Implement `active_character` switching.
- **Raw Tag:** Hold A1 for 15+ frames to tag. Departing character jumps out (vulnerable). Incoming character enters.
- **Assist Call:** Tap A1 to call the off-screen partner. The assist character enters, performs their assist move, and exits.
  - Assist cooldown system (180 frames normal, 300 frames if assist was hit).
  - Assist character has a hurtbox and takes 150% damage when hit.
- **DHC:** During a super's active frames, input the partner's super command + A1 to cancel into partner's super.
- **Snapback:** QCF + A1 (costs 1 bar). On hit, the opponent's point is forced out and assist is forced in.
- **Incoming Mixup:** When a character is KO'd, the replacement character enters from the top of the screen. The entering player can influence trajectory. 5 invincible frames on landing.
- **Blue Health:** When a character is tagged out, a portion of recent damage is shown as recoverable "blue" health. It regenerates at 5 HP/frame while off-screen.
- **Team KO:** The match ends when both characters on one team are KO'd.

**Acceptance Criteria:**
- [ ] Each player selects a team of 2 characters before the match.
- [ ] Pressing A1 briefly calls the assist. The assist character appears, attacks, and exits.
- [ ] Assist has a cooldown timer visible on the HUD. Cannot call assist during cooldown.
- [ ] Holding A1 tags out the point character and brings in the partner.
- [ ] The departing character is vulnerable during the tag-out jump. If hit, they are snapped back in.
- [ ] DHC works: during Ryker's Hadou Barrage, inputting Titan's super + A1 cancels into Titan's super.
- [ ] Snapback (QCF+A1) forces the opponent to switch characters on hit.
- [ ] When a character dies, the partner enters from the top as the incoming character.
- [ ] Blue health accumulates on the off-screen character and regenerates over time.
- [ ] The match correctly ends when both characters on one team are KO'd.

**Key Notes:**
- Assist calls and tags cannot happen during hitstun, blockstun, or while the assist is on cooldown or already on-screen.
- The off-screen character's state must be tracked (blue health recovery, cooldown timer) even though they are not rendered.
- DHC is essentially a forced tag during a super. The engine needs to handle super canceling mid-animation cleanly.

---

### Phase 9: UI / HUD

**Objective:** Implement the full in-match HUD with health bars, super meter, burst gauge, combo counter, timer, and Sparking Blast indicator.

**Deliverables:**
- `src/hud.h / hud.c`:
  - **Health Bars:** Two health bars per team (stacked: point character bar on top, assist character bar below, smaller). Show current HP (red), blue/recoverable HP (blue), and damage (decreasing yellow bar that drains to match red after a delay).
  - **Super Meter:** Bar at the bottom of the screen for each player. Divided into 5 segments. Fills smoothly. Glows when a bar is full.
  - **Burst Gauge:** Small circular or bar indicator near the super meter. Shows burst availability.
  - **Sparking Indicator:** Icon or text showing if Sparking Blast is available. When active, show a draining timer bar.
  - **Timer:** Centered at top of screen. Counts down from 99. Large, clear font.
  - **Combo Counter:** When a combo is active, display hit count and total damage near the defender. Fade out after combo ends.
  - **Assist Cooldown:** Small icon/timer near each player's side showing assist availability.
  - **Character Portraits:** Small portraits next to health bars identifying each character.

**Acceptance Criteria:**
- [ ] Health bars decrease smoothly when a character takes damage. The yellow "draining" portion trails behind.
- [ ] Blue health is visually distinct (blue color) and animates recovery when the character is off-screen.
- [ ] Super meter fills during attacks and being hit. Each full bar is visually distinct.
- [ ] Timer counts down and is clearly readable.
- [ ] Combo counter appears during combos, shows hit count and damage, and disappears (fades out) after the combo drops.
- [ ] Burst gauge indicator shows full/empty state and recharge progress.
- [ ] Sparking Blast indicator shows available/used state. When active, a timer bar drains.
- [ ] All HUD elements scale correctly and do not obscure gameplay.
- [ ] HUD renders on top of all game elements.

**Key Notes:**
- HUD state is derived from `GameState` -- it does not carry its own state (important for rollback).
- For the "draining" yellow health bar effect, store `display_hp` in a render-only struct (not in `GameState`) that lerps toward actual `hp` each render frame.
- Use a monospaced font for the timer and combo counter.

---

### Phase 10: GGPO Rollback Netcode

**Objective:** Integrate GGPO for online play with rollback netcode. The game must support 2-player online matches with smooth rollback up to 8 frames.

**Deliverables:**
- `src/network.h / network.c`:
  - GGPO session lifecycle: create session, add players (local/remote), synchronize, disconnect.
  - Lobby system: simple text-based connection screen where Player 1 hosts and Player 2 connects via IP:port.
  - Network status display: ping, rollback frame count, connection quality.
- `src/rollback.h / rollback.c`:
  - `save_game_state()`: serializes `GameState` to a byte buffer using `memcpy` (the struct is flat/pointer-free).
  - `load_game_state()`: deserializes a byte buffer back into `GameState`.
  - `free_buffer()`: frees serialized state buffers.
  - Checksum calculation for desync detection (simple CRC32 or Fletcher-32 over the state buffer).
- Modify `main.c` game loop to use GGPO's `ggpo_idle()` and `ggpo_advance_frame()` instead of direct `game_update()` calls when in online mode.
- Ensure audio does not play during rollback frames (check GGPO's `GGPO_EVENTCODE_TIMESYNC` and frame flags).
- Local mode (offline) still works without GGPO (direct `game_update()` calls).
- Spectator support (optional, GGPO provides this).

**Acceptance Criteria:**
- [ ] Two instances of the game can connect over a local network (or localhost) via IP:port.
- [ ] The match plays identically on both machines. Desyncs are detected via checksum comparison.
- [ ] Rollback is visually smooth: up to ~3 frames of rollback is imperceptible. 4-8 frames shows slight visual corrections but gameplay remains smooth.
- [ ] Input delay is configurable (default 2 frames). Rollback window is configurable (default 8 frames max).
- [ ] The game handles disconnection gracefully (timeout, display message, return to menu).
- [ ] `GameState` serialization/deserialization is verified: saving and loading state produces an identical simulation (write a test that runs 1000 frames, saves, loads, runs 100 more frames, and compares checksum with a reference run).
- [ ] No floating-point math exists in `game_update()` or any function called by it.
- [ ] Audio does not stutter or double-play during rollback.
- [ ] Network status (ping, rollback frames) is displayed on the HUD.

**Key Notes:**
- The MOST IMPORTANT prerequisite for GGPO is determinism. Before integrating GGPO, verify determinism: run the same match with the same inputs twice and confirm the game state is identical frame-by-frame. The checksum system helps here.
- `GameState` must not contain pointers. Use indices and fixed-size arrays. If any pointer sneaks in, serialization will break.
- GGPO library: use the open-source GGPO (https://github.com/pond3r/ggpo). It provides a C API. Build it as a static library and link it.
- Consider also evaluating `ggpo-rs` or other alternatives if the original GGPO has build issues. The API contract is the same.
- Input delay of 2 frames + up to 8 frames rollback gives a comfortable online experience for connections up to ~150ms RTT.

---

### Phase 11: Menu System & Character Select

**Objective:** Implement the full menu flow: title screen, main menu, character select, stage select, and pause menu.

**Deliverables:**
- `src/menu.h / menu.c`:
  - **Title Screen:** Game logo, "Press Start" prompt. Transitions to Main Menu.
  - **Main Menu:** Options: "Local VS", "Online VS", "Training Mode", "Options", "Exit".
  - **Character Select:** Both players choose 2 characters (their team). Display character portraits/silhouettes. Confirm with a button. Show selected team order (point/assist).
  - **Stage Select:** Choose a stage (2-3 simple stages with different backgrounds). Can be random.
  - **Pause Menu:** Press Start during a match to pause. Options: "Resume", "Return to Character Select", "Return to Main Menu". Only available in offline mode.
  - **Online Lobby:** "Host" or "Join" screen. Host displays their IP and port. Join prompts for remote IP and port.
  - **Training Mode:** Infinite health, meter, and a training dummy with configurable behavior (stand, crouch, jump, block all, no block). Display frame data and hitboxes.
  - **Options:** Key bindings, audio volume, display options.
- Menu state machine: the game has a top-level state (`GAME_STATE_TITLE`, `GAME_STATE_MENU`, `GAME_STATE_CHAR_SELECT`, `GAME_STATE_MATCH`, etc.) that determines what is updated and rendered.

**Acceptance Criteria:**
- [ ] The game boots to a title screen. Pressing a button goes to the main menu.
- [ ] Main menu is navigable with arrow keys and enter/button. All options lead to the correct screens.
- [ ] Character select allows each player to pick 2 characters. Picks are visually confirmed.
- [ ] After character select, the match begins with the chosen teams.
- [ ] Pause menu works in offline mode. Resuming returns to the exact game state.
- [ ] Online lobby allows hosting and joining. After connection, proceeds to character select, then match.
- [ ] Training mode: dummy stands still, hitboxes are visible, frame data for the last move is displayed, meter and health are infinite.
- [ ] "Return to Main Menu" from any screen works without crashing or memory leaks.

**Key Notes:**
- Menus are rendered with Raylib's shape and text drawing functions. No complex UI framework needed.
- Menu transitions can be instant (no animations needed for MVP, but a fade-to-black is nice).
- Training mode is crucial for development and testing. It should be implemented early within this phase.

---

### Phase 12: Remaining Characters & Audio

**Objective:** Implement the remaining 4 characters (Zara, Viper, Aegis, Blaze) and the audio system.

**Deliverables:**
- `src/character_data/zara.c`: Zara's full move set (projectiles, traps, teleport).
- `src/character_data/viper.c`: Viper's full move set (dashes, mixups, flash kick).
- `src/character_data/aegis.c`: Aegis's full move set (barriers, counters, shield bash).
- `src/character_data/blaze.c`: Blaze's full move set (dual stances, fire/ice specials).
- `src/audio.h / audio.c`:
  - Load and manage sound effects (SFX) and background music (BGM).
  - SFX categories: hit impacts (light, medium, heavy), block sounds, whiff sounds, character-specific move sounds, super activation, KO, announcer calls.
  - BGM: 1 track per stage (2-3 tracks total). Loop seamlessly.
  - Volume controls: separate SFX and BGM sliders.
  - Audio is non-deterministic (not part of game state). Suppressed during rollback.
- Placeholder audio: use simple synthesized sounds (Raylib can generate basic waveforms) or free sound effects for the initial build. Each hit strength should sound distinct.

**Acceptance Criteria:**
- [ ] All 6 characters are playable with complete move sets as specified in Section 5.2.
- [ ] Zara's traps persist on the stage, have hitboxes, and are triggered by the opponent walking over them.
- [ ] Titan's 360 command grab works correctly with the motion input system.
- [ ] Viper's double air dash functions (can air dash twice per airborne period).
- [ ] Aegis's counter (214) catches an opponent's attack and auto-counters.
- [ ] Blaze can switch stances (22+S) and all specials change based on current stance.
- [ ] Audio plays for: attacks hitting, attacks being blocked, attacks whiffing, super activation, KO.
- [ ] BGM plays during matches and loops.
- [ ] No audio stuttering or popping during normal gameplay.
- [ ] Audio does not play during GGPO rollback frames.

**Key Notes:**
- Implement characters one at a time. For each character: define moves -> test in training mode -> verify against the frame data tables -> test in a match.
- Zara's traps and Aegis's barriers are stage-persistent objects. They need to be in `GameState` (affects rollback). Use a fixed-size array for stage objects.
- Blaze's stance switch changes which `MoveData` array is active for specials. Normals remain the same in both stances.

---

### Phase 13: Polish & Final Integration

**Objective:** Visual polish, gameplay balance pass, bug fixes, and final integration of all systems.

**Deliverables:**
- **Visual Polish:**
  - Replace colored rectangles with proper sprites (even if simple pixel art). Each character needs: idle, walk, crouch, jump, all normals, all specials (at minimum key frames), hitstun, blockstun, knockdown.
  - If sprites are not available, use more detailed geometric shapes (different colors per character, limb representation with rectangles).
  - Hit sparks / impact effects (particle system: spawn N particles on hit with random velocities, fade out over 15 frames).
  - Super flash effect (screen darkens, character portrait flashes).
  - Screen shake on heavy hits and supers.
  - KO slow-motion effect (last hit of a KO plays at 25% speed for 30 frames).
- **Gameplay Balance:**
  - Review all frame data for each character. Ensure no infinites (infinite combos).
  - Verify hitstun decay properly prevents touch-of-death combos (outside of high-resource expenditure like 5 meter + Sparking).
  - Test each character against every other character for obvious imbalances.
  - Tune damage values so that a standard BnB combo deals 25-35% of a character's HP.
- **Bug Fixes:**
  - Corner behavior (characters should not clip through walls).
  - Assist interactions (assist should not be hit after leaving the screen).
  - DHC transitions should be seamless.
  - Desync testing: run extended online matches and verify checksum consistency.
- **Performance:**
  - Profile the game. Ensure `game_update()` runs in under 1ms (critical for rollback -- GGPO may need to re-simulate up to 8 frames in a single render frame).
  - Ensure rendering is under 8ms at 720p.
  - Memory: total memory usage should be under 256MB.
- **Quality of Life:**
  - Display input history on screen (training mode).
  - Frame data display in training mode.
  - Record/playback in training mode (record a sequence of inputs for the dummy, play it back).

**Acceptance Criteria:**
- [ ] All 6 characters have distinct visuals (sprites or detailed shapes).
- [ ] Hit effects play on every successful hit.
- [ ] Screen shake occurs on heavy attacks and supers.
- [ ] Super flash darkens the screen and pauses the game briefly.
- [ ] No infinite combos exist without spending resources.
- [ ] Standard BnB combos deal 25-35% damage. High-resource combos deal 50-70% maximum.
- [ ] No visual glitches in corners or at stage boundaries.
- [ ] Online play is stable for matches up to 5 minutes with no desync.
- [ ] Game runs at locked 60 FPS on modest hardware (integrated GPU, modern CPU).
- [ ] Training mode displays frame data and input history.
- [ ] The full game flow works end-to-end: title -> menu -> char select -> match -> result -> rematch or menu.

---

## Appendix A: Key Constants

```c
// Screen
#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       720
#define GAME_FPS            60

// Stage
#define STAGE_WIDTH         INT_TO_FP(1600)
#define STAGE_FLOOR_Y       INT_TO_FP(0)

// Physics
#define GRAVITY             INT_TO_FP(1)
#define JUMP_VELOCITY       INT_TO_FP(-18)
#define SUPER_JUMP_VELOCITY INT_TO_FP(-24)
#define WALK_SPEED_FORWARD  INT_TO_FP(4)
#define WALK_SPEED_BACKWARD INT_TO_FP(3)
#define DASH_SPEED          INT_TO_FP(6)
#define AIR_DASH_SPEED      INT_TO_FP(8)

// Combat
#define MAX_HP              10000
#define MAX_METER           5000
#define METER_PER_BAR       1000
#define MAX_BURST           1000
#define BURST_RECHARGE_RATE 1        // Per frame (1000/1 = 1000 frames ≈ 16.7 seconds)
#define BLUE_HP_REGEN_RATE  5        // Per frame while off-screen
#define HITSTOP_LIGHT       8
#define HITSTOP_MEDIUM      10
#define HITSTOP_HEAVY       12
#define HITSTOP_SUPER       15

// Input
#define INPUT_BUFFER_SIZE   60       // Frames of input history
#define MOTION_WINDOW_QCF   12       // Frames for QCF detection
#define MOTION_WINDOW_DP    15       // Frames for DP detection
#define MOTION_WINDOW_360   20       // Frames for 360 detection
#define BUFFER_WINDOW       8        // Input buffer leniency
#define DASH_WINDOW         10       // Double-tap detection window

// Tag System
#define ASSIST_COOLDOWN     180      // Frames
#define ASSIST_HIT_COOLDOWN 300      // Frames (if assist was hit)
#define ASSIST_DAMAGE_MULT  150      // Percent
#define TAG_HOLD_FRAMES     15       // Frames A1 must be held to tag
#define BLUE_HP_FRACTION    70       // Percent of damage that becomes blue HP

// Sparking Blast
#define SPARKING_DURATION_2 600      // Frames (2 characters alive)
#define SPARKING_DURATION_1 900      // Frames (1 character alive)
#define SPARKING_DAMAGE_MULT 110     // Percent

// Damage Scaling (indexed by hit number, 0-indexed)
static const int DAMAGE_SCALING[] = {
    100, 98, 96, 92, 88, 84, 80, 76, 72, 72, 72, 72, 72, 72, 72, 72
};
#define MIN_SCALING         72
#define LIGHT_STARTER_MULT  80       // Percent
#define SUPER_MIN_DAMAGE    30       // Percent of base
```

## Appendix B: Keyboard Mappings (Default)

| Action       | Player 1 | Player 2 |
|-------------|----------|----------|
| Up          | W        | Up Arrow |
| Down        | S        | Down Arrow |
| Left        | A        | Left Arrow |
| Right       | D        | Right Arrow |
| Light (L)   | U        | Numpad 4 |
| Medium (M)  | I        | Numpad 5 |
| Heavy (H)   | O        | Numpad 6 |
| Assist (A1) | P        | Numpad 0 |
| Start       | Enter    | Numpad Enter |

Gamepad (if connected): D-pad/left stick for directions, face buttons for L/M/H/A1 (Xbox: X/Y/B/RB, PlayStation: Square/Triangle/Circle/R1).

## Appendix C: State Machine Diagram (Simplified ASCII)

```
                        +-----------+
                        |   IDLE    |<--------------------------+
                        +-----+-----+                           |
                              |                                 |
         +--------+-----------+-----------+--------+            |
         |        |           |           |        |            |
         v        v           v           v        v            |
     WALK_F   WALK_B      CROUCH    JUMP_SQUAT  DASH          |
                              |           |                     |
                              v           v                     |
                         CROUCH_ATK   AIRBORNE--+               |
                                          |     |               |
                                          v     v               |
                                       LANDING AIR_ATK          |
                                          |                     |
                                          +---------------------+
         +--------+
         | ATTACK |  (STARTUP -> ACTIVE -> RECOVERY)
         +---+----+
             |
    on hit:  +---> chain cancel --> ATTACK
             +---> special cancel --> SPECIAL
             +---> super cancel --> SUPER
             +---> jump cancel --> JUMP_SQUAT (if sparking)
             |
    on block: same cancels, but defender enters BLOCKSTUN
             |
    whiff:   no cancel (full recovery)


         +---------+           +-----------+
         | HITSTUN |           | BLOCKSTUN |
         +----+----+           +-----+-----+
              |                      |
              v                      v
         KNOCKDOWN              IDLE (after blockstun expires)
              |
              v
         WAKEUP --> IDLE
```

## Appendix D: Development Time Estimates

| Phase | Description                    | Estimated Effort |
|-------|--------------------------------|------------------|
| 1     | Project Setup                  | 1 session        |
| 2     | Game Loop & Rendering          | 1-2 sessions     |
| 3     | Character State Machine        | 2-3 sessions     |
| 4     | Input System                   | 2-3 sessions     |
| 5     | Hitbox/Hurtbox & Collision     | 2-3 sessions     |
| 6     | Combo System & Frame Data      | 4-6 sessions     |
| 7     | Second Character               | 2-3 sessions     |
| 8     | Tag Team Mechanics             | 3-4 sessions     |
| 9     | UI/HUD                         | 2-3 sessions     |
| 10    | GGPO Rollback Netcode          | 3-5 sessions     |
| 11    | Menu System & Char Select      | 2-3 sessions     |
| 12    | Remaining Characters & Audio   | 4-6 sessions     |
| 13    | Polish                         | 3-5 sessions     |
| **Total** |                            | **~31-47 sessions** |

(A "session" here refers to one focused AI coding session of substantial work.)

---

*End of Plan Document*
