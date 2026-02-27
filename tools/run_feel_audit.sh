#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <capture_video.mp4>"
    exit 1
fi

VIDEO="$1"
if [[ ! -f "$VIDEO" ]]; then
    echo "Error: video not found: $VIDEO"
    exit 1
fi

TRACE_FILE="combat_trace.csv"
if [[ ! -f "$TRACE_FILE" ]]; then
    if [[ -f build/combat_trace.csv ]]; then
        TRACE_FILE="build/combat_trace.csv"
    else
        echo "Error: combat_trace.csv not found."
        echo "Expected one of:"
        echo "  - ./combat_trace.csv"
        echo "  - ./build/combat_trace.csv"
        echo "Run the game first with trace enabled."
        echo "Example: cd /Users/tommy.bonderenka/Documents/GitHub/vortex-clash && VORTEX_TRACE=1 ./build/vortex_clash"
        exit 1
    fi
fi

base="$(basename "$VIDEO")"
stem="${base%.*}"
out_dir="captures/${stem}_frames"
report_path="captures/${stem}_audit.md"

mkdir -p captures

echo "[1/3] Extracting frames and contact sheet..."
./tools/feel_audit_frames.sh "$VIDEO" "$out_dir"

echo "[2/3] Generating trace report..."
./tools/feel_trace_report.py --trace "$TRACE_FILE" --out "$report_path"

echo "[3/3] Done."
echo "Frames: $out_dir"
echo "Report: $report_path"
