"""
Repack a raw sprite sheet (magenta bg) into a uniform-frame-size sheet
with a manifest for vortex-clash's sprite system.

Two modes:
  --dump    : Save each row as a separate PNG for visual inspection
  (default) : Repack into final sheet + manifest

Usage:
  python repack_sheet.py --dump     # inspect rows
  python repack_sheet.py            # produce final sheet
"""
from PIL import Image
import numpy as np
import os, sys

BASE = os.path.join(os.path.dirname(__file__), '..')
SHEET_PATH = os.path.join(BASE, 'assets', 'ryker', 'karin.png')
OUT_SHEET  = os.path.join(BASE, 'assets', 'ryker', 'sheet.png')
OUT_MANIFEST = os.path.join(BASE, 'assets', 'ryker', 'manifest.txt')
DUMP_DIR   = os.path.join(BASE, 'tools', 'row_dumps')


def find_frames_in_row(bg_mask, y_start, y_end):
    row_bg = bg_mask[y_start:y_end, :]
    col_has = (~row_bg).any(axis=0)
    frames = []
    start = None
    gap = 0
    for x in range(len(col_has)):
        if col_has[x]:
            if start is None:
                start = x
            gap = 0
        else:
            if start is not None:
                gap += 1
                if gap > 3:
                    end = x - gap
                    if end - start > 5:
                        frames.append((start, end + 1))
                    start = None
                    gap = 0
    if start is not None:
        end = len(col_has) - 1
        while end > start and not col_has[end]:
            end -= 1
        if end - start > 5:
            frames.append((start, end + 1))

    # Split any abnormally wide frames (two sprites merged with tiny gap)
    MAX_SINGLE_FRAME_W = 140
    split_frames = []
    for (fs, fe) in frames:
        w = fe - fs
        if w > MAX_SINGLE_FRAME_W:
            # Find bg gap columns in the middle region to split on
            mid_region = col_has[fs:fe]
            best_gap_start = None
            best_gap_len = 0
            gs = None
            gl = 0
            # Search in the middle 60% of the frame
            search_start = int(w * 0.2)
            search_end = int(w * 0.8)
            for lx in range(search_start, search_end):
                if not mid_region[lx]:
                    if gs is None:
                        gs = lx
                    gl += 1
                else:
                    if gs is not None and gl > best_gap_len:
                        best_gap_start = gs
                        best_gap_len = gl
                    gs = None
                    gl = 0
            if gs is not None and gl > best_gap_len:
                best_gap_start = gs
                best_gap_len = gl
            if best_gap_len >= 1:
                split_x = fs + best_gap_start
                if split_x - fs > 5:
                    split_frames.append((fs, split_x))
                if fe - (split_x + best_gap_len) > 5:
                    split_frames.append((split_x + best_gap_len, fe))
            else:
                split_frames.append((fs, fe))
        else:
            split_frames.append((fs, fe))
    return split_frames


def find_label_bars(row_content, min_height=14, threshold=1500):
    """Find full-width label bars that separate animations."""
    bars = []
    in_label = False
    count = 0
    start = 0
    for y in range(len(row_content)):
        if row_content[y] > threshold:
            if not in_label:
                start = y
                in_label = True
                count = 1
            else:
                count += 1
        else:
            if in_label and count >= min_height:
                bars.append((start, start + count))
            in_label = False
            count = 0
    if in_label and count >= min_height:
        bars.append((start, start + count))
    return bars


def load_and_analyze():
    img = Image.open(SHEET_PATH).convert('RGBA')
    data = np.array(img)
    # Detect background color from top-left pixel
    bg_r, bg_g, bg_b = int(data[0,0,0]), int(data[0,0,1]), int(data[0,0,2])
    print(f"Detected BG color: ({bg_r}, {bg_g}, {bg_b})")
    # Exact bg mask (for label bar detection — labels are non-bg)
    tol = 10
    si = data[:,:,:3].astype(int)
    bg_exact = ((np.abs(si[:,:,0] - bg_r) < tol) &
                (np.abs(si[:,:,1] - bg_g) < tol) &
                (np.abs(si[:,:,2] - bg_b) < tol))

    # Use label bars as animation dividers (labels show as non-bg in exact mask)
    row_content = (~bg_exact).sum(axis=1)
    label_bars = find_label_bars(row_content)

    # Extended bg mask for sprite analysis (bg + green-dominant pixels)
    bg_green = ((si[:,:,1] > si[:,:,0] + 20) &
                (si[:,:,1] > si[:,:,2] + 20) &
                (si[:,:,1] > 100))
    bg_mask = bg_exact | bg_green
    print(f"Found {len(label_bars)} label bars")

    all_rows = {}  # keyed by label bar index
    for ri, (ls, le) in enumerate(label_bars):
        # Animation region: from end of this label to start of next (or end of image)
        anim_start = le
        anim_end = label_bars[ri + 1][0] if ri + 1 < len(label_bars) else data.shape[0]
        if anim_end - anim_start < 10:
            continue  # skip empty sections

        fxs = find_frames_in_row(bg_mask, anim_start, anim_end)
        sprites = []
        for fx_s, fx_e in fxs:
            fw = fx_e - fx_s
            if fw > 400 or fw < 20:  # skip oversized garbage and tiny fragments
                continue
            region_bg = bg_mask[anim_start:anim_end, fx_s:fx_e]
            # Tight-crop: trim bg rows from top/bottom
            rh = (~region_bg).any(axis=1)
            t = 0
            while t < len(rh) and not rh[t]: t += 1
            b = len(rh) - 1
            while b > t and not rh[b]: b -= 1
            fh = b - t + 1
            if fh < 10:  # skip tiny fragments
                continue
            # Tight-crop: trim bg cols from left/right
            cropped_bg = region_bg[t:b+1, :]
            ch = (~cropped_bg).any(axis=0)
            l = 0
            while l < len(ch) and not ch[l]: l += 1
            r = len(ch) - 1
            while r > l and not ch[r]: r -= 1
            sprites.append({'x': fx_s + l, 'y': anim_start + t, 'w': r - l + 1, 'h': fh})
        if sprites:
            all_rows[ri] = {'row_idx': ri, 'ys': anim_start, 'ye': anim_end, 'sprites': sprites}
    return data, bg_mask, all_rows, (bg_r, bg_g, bg_b)


def cmd_dump():
    data, bg_mask, all_rows, bg_color = load_and_analyze()
    bg_r, bg_g, bg_b = bg_color
    os.makedirs(DUMP_DIR, exist_ok=True)
    for ri in sorted(all_rows.keys()):
        row = all_rows[ri]
        sprites = row['sprites']
        # find max height in this row
        max_h = max(s['h'] for s in sprites)
        cell_w = max(s['w'] for s in sprites) + 4
        cell_h = max_h + 4
        strip_w = cell_w * len(sprites)
        strip = np.zeros((cell_h, strip_w, 4), dtype=np.uint8)
        for fi, s in enumerate(sprites):
            src = data[s['y']:s['y']+s['h'], s['x']:s['x']+s['w']].copy()
            tol = 30
            si = src[:,:,:3].astype(int)
            # Remove main bg color
            m = ((np.abs(si[:,:,0] - bg_r) < tol) &
                 (np.abs(si[:,:,1] - bg_g) < tol) &
                 (np.abs(si[:,:,2] - bg_b) < tol))
            # Also remove any green-dominant pixels (sub-labels, borders)
            m2 = ((si[:,:,1] > si[:,:,0] + 20) &
                  (si[:,:,1] > si[:,:,2] + 20) &
                  (si[:,:,1] > 100))
            src[m | m2] = [0,0,0,0]
            ox = fi*cell_w + (cell_w - s['w'])//2
            oy = cell_h - s['h']
            strip[oy:oy+s['h'], ox:ox+s['w']] = src
        out = Image.fromarray(strip, 'RGBA')
        path = os.path.join(DUMP_DIR, f'row_{ri:02d}_{len(sprites)}frames.png')
        out.save(path)
        print(f"Row {ri:2d}: {len(sprites):2d} frames  h={row['ye']-row['ys']}  -> {path}")
    print(f"\nDumped {len(all_rows)} rows to {DUMP_DIR}/")


def cmd_repack():
    data, bg_mask, all_rows, bg_color = load_and_analyze()
    bg_r, bg_g, bg_b = bg_color

    print(f"Rows detected: {len(all_rows)}")
    for r in all_rows.values():
        print(f"  Row {r['row_idx']:2d}: {len(r['sprites']):2d} frames  "
              f"y={r['ys']}..{r['ye']}  widths={[s['w'] for s in r['sprites'][:8]]}...")

    # ---------------------------------------------------------------
    # Karin (Street Fighter Alpha 3) — label-based animation mapping
    #
    #  0: STANCE / STANCE TURN    1: WALK FORWARD        2: WALK BACKWARD
    #  3: JAB                     4: STRONG               5: FIERCE
    #  6: SHORT                   7: FORWARD              8: ROUNDHOUSE
    #  9: CROUCH                 10: CROUCH JAB          11: CROUCH STRONG
    # 12: CROUCH FIERCE          13: CROUCH SHORT        14: CROUCH FORWARD
    # 15: CROUCH ROUNDHOUSE      16: JUMP                17: JUMP BACKWARD
    # 18: JUMP JAB               19: JUMP STRONG         20: JUMP FIERCE
    # 21: JUMP SHORT             22: JUMP FORWARD        23: JUMP ROUNDHOUSE
    # 24-52: specials/supers/grabs
    # 53: HEAD HIT / BODY HIT   54: KNOCKED DOWN
    # 55: KO/GET UP             56: GUARD               57: GUARD BREAK
    # 58+: intros/victories/portraits
    #
    # SF notation: Jab=LP, Strong=MP, Fierce=HP, Short=LK, Forward=MK, Roundhouse=HK
    # Our 3-button: L=Jab/Short, M=Strong/Forward, H=Fierce/Roundhouse
    # Using punches for standing/crouching, keeping kicks as alt option
    # ---------------------------------------------------------------
    animations = [
        # (name,        src_row, start, count, duration, loop)
        ('idle',         0,       0,   7,  8, 1),   # STANCE (skip turn frames at end)
        ('walk_fwd',     1,       0,  None, 5, 1),   # WALK FORWARD
        ('walk_back',    2,       0,  None, 5, 1),   # WALK BACKWARD
        ('crouch',       9,       0,   2,  4, 0),   # CROUCH (skip label text frames)
        ('jump',        16,       0,   5,  4, 0),   # JUMP (neutral jump only)
        ('dash_fwd',     1,       0,  None, 3, 0),   # reuse walk fwd for dash
        ('dash_back',    2,       0,  None, 3, 0),   # reuse walk back for dash
        ('5L',           3,       0,  None, 2, 0),   # JAB
        ('5M',           4,       0,  None, 2, 0),   # STRONG
        ('5H',           5,       0,  None, 3, 0),   # FIERCE
        ('2L',          10,       0,  None, 2, 0),   # CROUCH JAB
        ('2M',          11,       0,  None, 2, 0),   # CROUCH STRONG
        ('2H',          12,       0,  None, 3, 0),   # CROUCH FIERCE
        ('jL',          18,       0,  None, 2, 0),   # JUMP JAB
        ('jM',          19,       0,  None, 2, 0),   # JUMP STRONG
        ('jH',          20,       0,  None, 3, 0),   # JUMP FIERCE
        ('hitstun',     45,       0,   6,  3, 0),   # Standing hit reactions (head snap, recoil)
        ('blockstun',   56,       0,  None, 3, 0),   # Guard stun
        ('knockdown',   46,       0,   6,  4, 0),   # Knocked down / falling
    ]

    # Resolve frame lists
    anim_data = []
    max_w = 0
    max_h = 0
    for name, src_row, start, count, dur, loop in animations:
        row = all_rows[src_row]
        sprites = row['sprites']
        if count is None:
            count = len(sprites) - start
        count = min(count, len(sprites) - start)
        if count <= 0:
            print(f"  WARNING: {name} has 0 frames, skipping")
            continue
        frames = sprites[start:start+count]
        for f in frames:
            if f['w'] > max_w: max_w = f['w']
            if f['h'] > max_h: max_h = f['h']
        anim_data.append((name, frames, count, dur, loop))

    cell_w = max_w + 4
    cell_h = max_h + 4
    max_count = max(c for _, _, c, _, _ in anim_data)
    sheet_w = cell_w * max_count
    sheet_h = cell_h * len(anim_data)

    print(f"\nCell: {cell_w}x{cell_h}  Sheet: {sheet_w}x{sheet_h}")
    print(f"Total animations: {len(anim_data)}, max frames/row: {max_count}")

    out = np.zeros((sheet_h, sheet_w, 4), dtype=np.uint8)
    manifest = ["# anim_name  x  y  frame_w  frame_h  frame_count  frame_duration  looping"]

    for row_i, (name, frames, count, dur, loop) in enumerate(anim_data):
        dy = row_i * cell_h
        for fi, f in enumerate(frames):
            dx = fi * cell_w
            src = data[f['y']:f['y']+f['h'], f['x']:f['x']+f['w']].copy()
            tol = 30
            si = src[:,:,:3].astype(int)
            # Remove main bg color
            m = ((np.abs(si[:,:,0] - bg_r) < tol) &
                 (np.abs(si[:,:,1] - bg_g) < tol) &
                 (np.abs(si[:,:,2] - bg_b) < tol))
            # Also remove any green-dominant pixels (sub-labels, borders)
            m2 = ((si[:,:,1] > si[:,:,0] + 20) &
                  (si[:,:,1] > si[:,:,2] + 20) &
                  (si[:,:,1] > 100))
            src[m | m2] = [0,0,0,0]
            ox = (cell_w - f['w']) // 2
            oy = cell_h - f['h']  # bottom-align
            out[dy+oy:dy+oy+f['h'], dx+ox:dx+ox+f['w']] = src
        manifest.append(f"{name:<12s} 0    {dy:<4d} {cell_w:<8d} {cell_h:<8d} "
                        f"{count:<12d} {dur:<16d} {loop}")
        print(f"  {name}: {count} frames at y={dy}")

    Image.fromarray(out, 'RGBA').save(OUT_SHEET)
    print(f"\nSaved: {OUT_SHEET}")
    with open(OUT_MANIFEST, 'w') as f:
        f.write('\n'.join(manifest) + '\n')
    print(f"Saved: {OUT_MANIFEST}")


if __name__ == '__main__':
    if '--dump' in sys.argv:
        cmd_dump()
    else:
        cmd_repack()
