#!/usr/bin/env bash
# Renders radar preview images from the real src/ui/ render code and writes PNGs to
# docs/images/. Run from the repo root:
#
#   tools/render_preview/render.sh
#
# Requires a C++17 compiler and Python with Pillow. Preview tooling only — nothing here
# is compiled into the firmware.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOL="$ROOT/tools/render_preview"
OUT="$ROOT/docs/images"
BIN="$(mktemp -d)/render_preview"

mkdir -p "$OUT"

# -I$TOOL first so its LovyanGFX.hpp shadows the real one.
c++ -std=c++17 -O1 -o "$BIN" \
    -I"$TOOL" -I"$ROOT/include" -I"$ROOT/src" \
    "$TOOL/main.cpp" \
    "$ROOT/src/ui/radar_render.cpp" \
    "$ROOT/src/ui/projection.cpp" \
    "$ROOT/src/model/aircraft.cpp" \
    "$ROOT/src/model/units.cpp" \
    "$ROOT/src/model/geo.cpp" \
    "$ROOT/src/data/airports.cpp"

# One image per range preset, so the README can show the same sky at two zooms.
for km in 10 40; do
    "$BIN" "$km" > "$BIN-$km.ppm"
    python3 "$TOOL/to_png.py" "$BIN-$km.ppm" "$OUT/radar-${km}km.png"
    echo "wrote docs/images/radar-${km}km.png"
done
