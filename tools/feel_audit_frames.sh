#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 <input_video.mp4> [output_dir]"
    echo "Example: $0 captures/session01.mp4 captures/session01_frames"
    exit 1
fi

if ! command -v ffmpeg >/dev/null 2>&1; then
    echo "Error: ffmpeg is required (brew install ffmpeg)"
    exit 1
fi

INPUT_VIDEO="$1"
if [[ ! -f "$INPUT_VIDEO" ]]; then
    echo "Error: input file not found: $INPUT_VIDEO"
    exit 1
fi

if [[ $# -eq 2 ]]; then
    OUT_DIR="$2"
else
    base="$(basename "$INPUT_VIDEO")"
    stem="${base%.*}"
    OUT_DIR="captures/${stem}_frames"
fi

mkdir -p "$OUT_DIR"

echo "Extracting 60fps frames to $OUT_DIR ..."
ffmpeg -hide_banner -loglevel error -y \
    -i "$INPUT_VIDEO" \
    -vf "fps=60" \
    "$OUT_DIR/frame_%06d.png"

echo "Generating contact sheet ..."
ffmpeg -hide_banner -loglevel error -y \
    -i "$INPUT_VIDEO" \
    -vf "fps=2,scale=320:-1,tile=8x8" \
    -frames:v 1 \
    "$OUT_DIR/contact_sheet.png"

echo "Done."
echo "Frames: $OUT_DIR/frame_*.png"
echo "Sheet:  $OUT_DIR/contact_sheet.png"
