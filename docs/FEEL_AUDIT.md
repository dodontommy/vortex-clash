# Feel Audit Workflow

This guide is the repeatable workflow for diagnosing combo reliability, hit-flash quality, and throw behavior.

## Capture Requirements

- Format: MP4 (H.264/H.265 is fine)
- Framerate: **60fps**
- Visuals: no motion blur/post filter if possible
- Framing: full gameplay + HUD visible
- Include scenarios:
  - Ground confirms (`5L->5M->5H`, `2L->2M->2H`) at near and max spacing
  - Air route into knockdown + OTG attempt
  - Multiple throw attempts on both screen sides

## 1) Enable Combat Trace

Run the game with deterministic per-frame trace output:

```bash
VORTEX_TRACE=1 ./build/vortex_clash
```

This writes `combat_trace.csv` in the current working directory.

CSV fields:
- `frame`
- `p1_state`, `p2_state`
- `p1_move`, `p2_move` (`AttackRef` encoded as `type*100 + index`)
- `distance` (center-to-center pixels)
- `hitstop`
- `p1_combo`, `p2_combo`
- `p1_hitstun`, `p2_hitstun`
- `p1_blockstun`, `p2_blockstun`
- `p1_result`, `p2_result` (`0=none`, `1=hit`, `2=block`)

## 2) Split Video Into Frames

```bash
./tools/feel_audit_frames.sh captures/session01.mp4
```

Outputs:
- `captures/session01_frames/frame_*.png` (60fps frame-by-frame)
- `captures/session01_frames/contact_sheet.png` (quick scan)

## 2b) Generate Trace Report

```bash
./tools/feel_trace_report.py --trace combat_trace.csv --out captures/session01_audit.md
```

Or run everything after capture in one command:

```bash
./tools/run_feel_audit.sh captures/session01.mp4
```

## 3) Correlate Frames With Trace

- Find the visible impact frame in `frame_*.png`.
- Match to `combat_trace.csv` by rough timeline and hit events.
- Verify:
  - Confirm windows align with expected hitstop/hitstun
  - No random combo-drop transition while OTG window is valid
  - Throw connects resolve into throw lock/thrown/knockdown consistently
  - Hit flash has no additive fringe artifacts

## 4) Record Tuning Decisions

For each issue, record:
- Repro clip + frame number
- Trace frame row(s)
- Root-cause subsystem (`player`, `hitbox`, `game`, `sprite`)
- Exact constant/data change (frame counts, range, window, etc.)
