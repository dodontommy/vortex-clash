#!/usr/bin/env python3
"""Generate a gameplay feel audit report from combat_trace.csv."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List


FPS = 60.0

STATE = {
    0: "IDLE",
    1: "WALK_FORWARD",
    2: "WALK_BACKWARD",
    3: "CROUCH",
    4: "JUMP_SQUAT",
    5: "AIRBORNE",
    6: "LANDING",
    7: "DASH_FORWARD",
    8: "DASH_BACKWARD",
    9: "ATTACK_STARTUP",
    10: "ATTACK_ACTIVE",
    11: "ATTACK_RECOVERY",
    12: "THROW_LOCK",
    13: "THROWN",
    14: "HITSTUN",
    15: "BLOCKSTUN",
    16: "KNOCKDOWN",
    17: "TAG_DEPARTING",
    18: "ASSIST_ENTERING",
    19: "ASSIST_ACTIVE",
    20: "ASSIST_EXITING",
    21: "INCOMING_FALL",
}


@dataclass
class Row:
    frame: int
    p1_state: int
    p2_state: int
    p1_move: int
    p2_move: int
    distance: int
    hitstop: int
    p1_combo: int
    p2_combo: int
    p1_hitstun: int
    p2_hitstun: int
    p1_blockstun: int
    p2_blockstun: int
    p1_x: int
    p2_x: int
    p1_result: int
    p2_result: int


def parse_rows(path: Path) -> List[Row]:
    rows: List[Row] = []
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        for raw in reader:
            rows.append(
                Row(
                    frame=int(raw["frame"]),
                    p1_state=int(raw["p1_state"]),
                    p2_state=int(raw["p2_state"]),
                    p1_move=int(raw["p1_move"]),
                    p2_move=int(raw["p2_move"]),
                    distance=int(raw["distance"]),
                    hitstop=int(raw["hitstop"]),
                    p1_combo=int(raw["p1_combo"]),
                    p2_combo=int(raw["p2_combo"]),
                    p1_hitstun=int(raw["p1_hitstun"]),
                    p2_hitstun=int(raw["p2_hitstun"]),
                    p1_blockstun=int(raw["p1_blockstun"]),
                    p2_blockstun=int(raw["p2_blockstun"]),
                    p1_x=int(raw.get("p1_x", 0)),
                    p2_x=int(raw.get("p2_x", 0)),
                    p1_result=int(raw["p1_result"]),
                    p2_result=int(raw["p2_result"]),
                )
            )
    return rows


def secs(frame: int) -> float:
    return frame / FPS


def move_for_player(row: Row, player: int) -> int:
    return row.p1_move if player == 1 else row.p2_move


def result_for_player(row: Row, player: int) -> int:
    return row.p1_result if player == 1 else row.p2_result


def combo_for_player(row: Row, player: int) -> int:
    return row.p1_combo if player == 1 else row.p2_combo


def opp_state(row: Row, player: int) -> int:
    return row.p2_state if player == 1 else row.p1_state


def opp_hitstun(row: Row, player: int) -> int:
    return row.p2_hitstun if player == 1 else row.p1_hitstun


def find_route_hits(rows: List[Row], player: int, route: List[int]) -> List[Dict[str, int]]:
    hits = []
    for idx, row in enumerate(rows):
        if result_for_player(row, player) == 1:
            hits.append((idx, row.frame, move_for_player(row, player), row.distance))

    matches: List[Dict[str, int]] = []
    pos = 0
    while pos < len(hits):
        route_i = 0
        start = pos
        end = pos
        dsum = 0
        dcount = 0
        while end < len(hits) and route_i < len(route):
            _, fr, mv, dist = hits[end]
            if mv == route[route_i]:
                if route_i > 0:
                    prev_fr = hits[end - 1][1]
                    if fr - prev_fr > 45:
                        break
                route_i += 1
                dsum += dist
                dcount += 1
            end += 1
        if route_i == len(route):
            matches.append(
                {
                    "start_frame": hits[start][1],
                    "end_frame": hits[end - 1][1],
                    "avg_distance": int(dsum / dcount) if dcount else 0,
                }
            )
            pos = end
        else:
            pos = start + 1
    return matches


def collect_drops(rows: List[Row], player: int) -> List[Dict[str, int]]:
    drops: List[Dict[str, int]] = []
    for i in range(1, len(rows)):
        prev = rows[i - 1]
        cur = rows[i]
        prev_combo = combo_for_player(prev, player)
        cur_combo = combo_for_player(cur, player)
        if prev_combo >= 2 and cur_combo == 0:
            os = opp_state(cur, player)
            oh = opp_hitstun(cur, player)
            suspicious = oh > 0 or os in (13, 14, 15, 16)
            drops.append(
                {
                    "frame": cur.frame,
                    "prev_combo": prev_combo,
                    "opp_state": os,
                    "opp_hitstun": oh,
                    "suspicious": 1 if suspicious else 0,
                }
            )
    return drops


def collect_throw_stats(rows: List[Row], player: int) -> Dict[str, int]:
    attempts = 0
    connects = 0
    prev_throw_move = False
    prev_throw_lock = False

    for row in rows:
        mv = move_for_player(row, player)
        st = row.p1_state if player == 1 else row.p2_state
        throw_move = mv == 400 and st in (9, 10, 11)
        throw_lock = st == 12
        if throw_move and not prev_throw_move:
            attempts += 1
        if throw_lock and not prev_throw_lock:
            connects += 1
        prev_throw_move = throw_move
        prev_throw_lock = throw_lock

    return {"attempts": attempts, "connects": connects, "fails": max(0, attempts - connects)}


def collect_hitstop_events(rows: List[Row]) -> List[Dict[str, int]]:
    events: List[Dict[str, int]] = []
    if not rows:
        return events

    run_frame = rows[0].frame
    run_start = 0
    for i in range(1, len(rows) + 1):
        if i < len(rows) and rows[i].frame == run_frame:
            continue
        segment = rows[run_start:i]
        frozen_ticks = sum(1 for r in segment if r.hitstop > 0)
        if frozen_ticks > 0:
            events.append({"frame": run_frame, "freeze_ticks": frozen_ticks, "rows": len(segment)})
        if i < len(rows):
            run_frame = rows[i].frame
            run_start = i
    return events


def collect_otg_hits(rows: List[Row], player: int) -> List[int]:
    otg_frames: List[int] = []
    for i in range(1, len(rows)):
        cur = rows[i]
        prev = rows[i - 1]
        if result_for_player(cur, player) != 1:
            continue
        if opp_state(prev, player) == 16:
            otg_frames.append(cur.frame)
    return otg_frames


def strength_bucket_from_move(move_id: int) -> str:
    move_type = move_id // 100
    idx = move_id % 100

    if move_type == 1:  # normal
        if idx in (0, 3, 6, 9):
            return "L"
        if idx in (1, 4, 7, 10):
            return "M"
        return "H"
    if move_type == 2:  # special
        rem = idx % 3
        if rem == 0:
            return "L"
        if rem == 1:
            return "M"
        return "H"
    if move_type in (3, 4):  # supers / throws
        return "H"
    return "?"


def collect_displacement_by_strength(rows: List[Row], player: int) -> Dict[str, float]:
    acc: Dict[str, List[int]] = {"L": [], "M": [], "H": []}
    last_hit_frame = -1
    for i, cur in enumerate(rows):
        if result_for_player(cur, player) != 1:
            continue
        if cur.frame == last_hit_frame:
            continue
        last_hit_frame = cur.frame

        # Measure displacement over a short post-hit window (6 game frames)
        # so the metric reflects actual recoil/slide, not same-frame contact.
        start_opp_x = cur.p2_x if player == 1 else cur.p1_x
        target_frame = cur.frame + 6
        end_opp_x = start_opp_x
        for j in range(i + 1, len(rows)):
            future = rows[j]
            if future.frame >= target_frame:
                end_opp_x = future.p2_x if player == 1 else future.p1_x
                break
        disp = abs(end_opp_x - start_opp_x)
        bucket = strength_bucket_from_move(move_for_player(cur, player))
        if bucket in acc:
            acc[bucket].append(disp)
    out: Dict[str, float] = {}
    for key in ("L", "M", "H"):
        vals = acc[key]
        out[key] = float(sum(vals) / len(vals)) if vals else 0.0
    return out


def spacing_bucket(distance: int) -> str:
    if distance <= 120:
        return "near"
    if distance <= 220:
        return "mid"
    return "far"


def route_spacing_counts(matches: List[Dict[str, int]]) -> Dict[str, int]:
    counts = {"near": 0, "mid": 0, "far": 0}
    for m in matches:
        counts[spacing_bucket(m["avg_distance"])] += 1
    return counts


def route_frozen_ratio(rows: List[Row], matches: List[Dict[str, int]]) -> float:
    if not matches:
        return 0.0
    total = 0
    frozen = 0
    for m in matches:
        start = m["start_frame"]
        end = m["end_frame"]
        for r in rows:
            if start <= r.frame <= end:
                total += 1
                if r.hitstop > 0:
                    frozen += 1
    if total == 0:
        return 0.0
    return (frozen * 100.0) / total


def collect_hit_events(rows: List[Row], player: int) -> List[Dict[str, int]]:
    events: List[Dict[str, int]] = []
    last_frame = -1
    for row in rows:
        if result_for_player(row, player) != 1:
            continue
        if row.frame == last_frame:
            continue
        last_frame = row.frame
        events.append(
            {
                "frame": row.frame,
                "move": move_for_player(row, player),
                "distance": row.distance,
            }
        )
    return events


def route_confirm_success_by_spacing(
    events: List[Dict[str, int]], route: List[int], max_gap: int = 45
) -> Dict[str, Dict[str, int]]:
    out: Dict[str, Dict[str, int]] = {
        "near": {"attempts": 0, "success": 0},
        "mid": {"attempts": 0, "success": 0},
        "far": {"attempts": 0, "success": 0},
    }
    if not route:
        return out

    for i, ev in enumerate(events):
        if ev["move"] != route[0]:
            continue
        bucket = spacing_bucket(ev["distance"])
        out[bucket]["attempts"] += 1

        route_i = 1
        prev_frame = ev["frame"]
        for j in range(i + 1, len(events)):
            nxt = events[j]
            if nxt["frame"] - prev_frame > max_gap:
                break
            if nxt["move"] == route[route_i]:
                route_i += 1
                prev_frame = nxt["frame"]
                if route_i == len(route):
                    out[bucket]["success"] += 1
                    break
    return out


def format_confirm_rate(stats: Dict[str, Dict[str, int]]) -> str:
    parts = []
    for b in ("near", "mid", "far"):
        attempts = stats[b]["attempts"]
        success = stats[b]["success"]
        pct = (success * 100.0 / attempts) if attempts else 0.0
        parts.append(f"{b}:{success}/{attempts} ({pct:.0f}%)")
    return " ".join(parts)


def generate_report(rows: List[Row], trace_path: Path) -> str:
    total_frames = rows[-1].frame + 1 if rows else 0
    duration = secs(total_frames)
    p1_hits = sum(1 for r in rows if r.p1_result == 1)
    p2_hits = sum(1 for r in rows if r.p2_result == 1)
    p1_blocks = sum(1 for r in rows if r.p1_result == 2)
    p2_blocks = sum(1 for r in rows if r.p2_result == 2)
    p1_max_combo = max((r.p1_combo for r in rows), default=0)
    p2_max_combo = max((r.p2_combo for r in rows), default=0)

    p1_5lmh = find_route_hits(rows, 1, [100, 101, 102])
    p2_5lmh = find_route_hits(rows, 2, [100, 101, 102])
    p1_2lmh = find_route_hits(rows, 1, [103, 104, 105])
    p2_2lmh = find_route_hits(rows, 2, [103, 104, 105])

    p1_drops = collect_drops(rows, 1)
    p2_drops = collect_drops(rows, 2)
    p1_suspicious = sum(1 for d in p1_drops if d["suspicious"])
    p2_suspicious = sum(1 for d in p2_drops if d["suspicious"])

    p1_throw = collect_throw_stats(rows, 1)
    p2_throw = collect_throw_stats(rows, 2)
    p1_otg = collect_otg_hits(rows, 1)
    p2_otg = collect_otg_hits(rows, 2)
    hitstop_events = collect_hitstop_events(rows)
    frozen_rows = sum(1 for r in rows if r.hitstop > 0)
    frozen_pct = (frozen_rows * 100.0 / len(rows)) if rows else 0.0
    p1_disp = collect_displacement_by_strength(rows, 1)
    p2_disp = collect_displacement_by_strength(rows, 2)
    p1_routes = p1_5lmh + p1_2lmh
    p2_routes = p2_5lmh + p2_2lmh
    p1_route_frozen = route_frozen_ratio(rows, p1_routes)
    p2_route_frozen = route_frozen_ratio(rows, p2_routes)
    p1_spacing = route_spacing_counts(p1_5lmh)
    p2_spacing = route_spacing_counts(p2_5lmh)
    p1_events = collect_hit_events(rows, 1)
    p2_events = collect_hit_events(rows, 2)
    p1_confirm_5 = route_confirm_success_by_spacing(p1_events, [100, 101, 102])
    p2_confirm_5 = route_confirm_success_by_spacing(p2_events, [100, 101, 102])
    p1_confirm_2 = route_confirm_success_by_spacing(p1_events, [103, 104, 105])
    p2_confirm_2 = route_confirm_success_by_spacing(p2_events, [103, 104, 105])
    displacement_all_zero = (
        p1_hits + p2_hits > 0
        and p1_disp["L"] == 0.0
        and p1_disp["M"] == 0.0
        and p1_disp["H"] == 0.0
        and p2_disp["L"] == 0.0
        and p2_disp["M"] == 0.0
        and p2_disp["H"] == 0.0
    )

    lines: List[str] = []
    lines.append("# Feel Audit Report")
    lines.append("")
    lines.append(f"- Source trace: `{trace_path}`")
    lines.append(f"- Duration: `{duration:.2f}s` ({total_frames} frames)")
    lines.append("")
    lines.append("## Snapshot")
    lines.append(f"- P1 hits/blocks: `{p1_hits}` / `{p1_blocks}`")
    lines.append(f"- P2 hits/blocks: `{p2_hits}` / `{p2_blocks}`")
    lines.append(f"- Max combo: `P1={p1_max_combo}` `P2={p2_max_combo}`")
    lines.append(f"- Hitstop events observed: `{len(hitstop_events)}`")
    lines.append(f"- Frozen rows in trace: `{frozen_pct:.1f}%`")
    lines.append("")
    lines.append("## Ground Magic Series")
    lines.append(f"- P1 `5L->5M->5H` confirmations: `{len(p1_5lmh)}`")
    lines.append(f"- P2 `5L->5M->5H` confirmations: `{len(p2_5lmh)}`")
    lines.append(f"- P1 `2L->2M->2H` confirmations: `{len(p1_2lmh)}`")
    lines.append(f"- P2 `2L->2M->2H` confirmations: `{len(p2_2lmh)}`")
    lines.append(
        f"- `5L->5M->5H` spacing buckets (`near/mid/far`): P1=`{p1_spacing['near']}/{p1_spacing['mid']}/{p1_spacing['far']}`, "
        f"P2=`{p2_spacing['near']}/{p2_spacing['mid']}/{p2_spacing['far']}`"
    )
    if p1_5lmh or p2_5lmh or p1_2lmh or p2_2lmh:
        samples = (p1_5lmh + p2_5lmh + p1_2lmh + p2_2lmh)[:5]
        lines.append("- Sample routes (frame range, avg spacing):")
        for s in samples:
            lines.append(
                f"  - `{s['start_frame']}->{s['end_frame']}` ({secs(s['start_frame']):.2f}s->{secs(s['end_frame']):.2f}s), "
                f"distance~`{s['avg_distance']}`"
            )
    lines.append("")
    lines.append("## Combo Drop Scan")
    lines.append(f"- P1 drop points: `{len(p1_drops)}` (suspicious: `{p1_suspicious}`)")
    lines.append(f"- P2 drop points: `{len(p2_drops)}` (suspicious: `{p2_suspicious}`)")
    top_drops = (p1_drops[:3] + p2_drops[:3])[:6]
    if top_drops:
        lines.append("- First suspicious candidates:")
        for d in top_drops:
            st = STATE.get(d["opp_state"], str(d["opp_state"]))
            lines.append(
                f"  - frame `{d['frame']}` ({secs(d['frame']):.2f}s), prev_combo=`{d['prev_combo']}`, "
                f"opp_state=`{st}`, opp_hitstun=`{d['opp_hitstun']}`, suspicious=`{d['suspicious']}`"
            )
    lines.append("")
    lines.append("## Feel Budget Metrics")
    lines.append(f"- Route frozen-frame %: `P1={p1_route_frozen:.1f}%` `P2={p2_route_frozen:.1f}%`")
    lines.append(
        f"- Avg defender displacement (px/hit): P1 `L={p1_disp['L']:.1f}` `M={p1_disp['M']:.1f}` `H={p1_disp['H']:.1f}`"
    )
    lines.append(
        f"- Avg defender displacement (px/hit): P2 `L={p2_disp['L']:.1f}` `M={p2_disp['M']:.1f}` `H={p2_disp['H']:.1f}`"
    )
    lines.append(
        f"- Confirm success by spacing `5L->5M->5H`: P1 `{format_confirm_rate(p1_confirm_5)}`, P2 `{format_confirm_rate(p2_confirm_5)}`"
    )
    lines.append(
        f"- Confirm success by spacing `2L->2M->2H`: P1 `{format_confirm_rate(p1_confirm_2)}`, P2 `{format_confirm_rate(p2_confirm_2)}`"
    )
    if displacement_all_zero:
        lines.append("- Note: displacement is all zero; capture a fresh trace from the latest build (with `p1_x/p2_x` columns).")
    lines.append("")
    lines.append("## Air Route / OTG")
    lines.append(f"- P1 OTG-like hit frames (hit while opponent was in knockdown previous frame): `{len(p1_otg)}`")
    lines.append(f"- P2 OTG-like hit frames: `{len(p2_otg)}`")
    if p1_otg or p2_otg:
        frames = (p1_otg + p2_otg)[:8]
        lines.append("- OTG sample frames:")
        for fr in frames:
            lines.append(f"  - frame `{fr}` ({secs(fr):.2f}s)")
    lines.append("")
    lines.append("## Throw Reliability")
    lines.append(
        f"- P1 throw attempts/connects/fails: `{p1_throw['attempts']}/{p1_throw['connects']}/{p1_throw['fails']}`"
    )
    lines.append(
        f"- P2 throw attempts/connects/fails: `{p2_throw['attempts']}/{p2_throw['connects']}/{p2_throw['fails']}`"
    )
    lines.append("")
    lines.append("## What To Inspect In Video")
    lines.append("- Validate hit flash artifacting at each hit frame window from this report.")
    lines.append("- Check visual continuity during throw lock/thrown sequence.")
    lines.append("- Compare near-spacing vs far-spacing confirm consistency on listed routes.")
    lines.append("")
    lines.append("## Notes")
    lines.append("- `p*_result`: `0=none`, `1=hit`, `2=block`.")
    lines.append("- Move IDs use `AttackRef` encoding (`type*100 + index`), throw currently appears as `400`.")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate gameplay feel report from combat trace CSV.")
    parser.add_argument("--trace", type=Path, default=Path("combat_trace.csv"), help="Path to combat trace CSV")
    parser.add_argument("--out", type=Path, default=None, help="Output markdown path")
    args = parser.parse_args()

    if not args.trace.exists():
        print(f"error: trace file not found: {args.trace}")
        return 2

    rows = parse_rows(args.trace)
    out_path = args.out if args.out else args.trace.with_name(args.trace.stem + "_report.md")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(generate_report(rows, args.trace), encoding="utf-8")
    print(f"wrote: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
