#!/usr/bin/env bash
# Builds the HUD simulator and the screen behavior tests: the real firmware
# screen code plus the U8g2 graphics library, compiled for your computer.
# Pass --test to run the tests too.
#
# Needs: a C/C++ compiler, and the U8g2 library that PlatformIO downloads
# (run any firmware build once, e.g. `pio run -e feather_demo`).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
FW="$ROOT/firmware"
U8G2=${U8G2_DIR:-$(ls -d "$FW"/.pio/libdeps/*/U8g2/src 2>/dev/null | head -1)}
OUT=${OUT:-$ROOT/build/hudsim}

if [ -z "$U8G2" ] || [ ! -d "$U8G2/clib" ]; then
  echo "U8g2 not found. Run a firmware build first (cd firmware && pio run -e feather_demo)." >&2
  exit 1
fi

mkdir -p "$OUT/obj"
# The U8g2 C files only need compiling once (the font file is big and slow).
ls "$U8G2"/clib/*.c | xargs -P "$(nproc 2>/dev/null || echo 4)" -I{} sh -c '
  o="'"$OUT"'/obj/$(basename "{}" .c).o"
  [ -f "$o" ] || cc -O1 -c "{}" -I"'"$U8G2"'/clib" -o "$o"
'

CXX="c++ -std=gnu++17 -O1 -Wall -Wextra -I$ROOT/tools/hudsim/fake -I$U8G2 -I$U8G2/clib -I$FW/src -I$FW/lib/core"
$CXX "$ROOT/tools/hudsim/sim_main.cpp" "$FW/src/hud.cpp" "$FW/src/demo.cpp" "$FW"/lib/core/*.cpp \
  "$OUT"/obj/*.o -o "$OUT/hudsim"
$CXX "$ROOT/tools/hudsim/hud_test.cpp" "$FW/src/hud.cpp" "$FW"/lib/core/*.cpp \
  "$OUT"/obj/*.o -o "$OUT/hud_test"
echo "Built $OUT/hudsim and $OUT/hud_test"

# --test: also run the screen behavior tests.
if [ "${1:-}" = "--test" ]; then
  "$OUT/hud_test"
fi
