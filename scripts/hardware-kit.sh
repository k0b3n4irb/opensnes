#!/bin/sh
# Collect the ROMs of docs/HARDWARE_VERIFICATION.md into release/hardware-kit/,
# numbered in grid order, ready to copy to a flash cart's SD card. The list is
# read from the protocol's table so the doc and the kit cannot drift apart.
set -eu
here=$(cd "$(dirname "$0")/.." && pwd)
doc="$here/docs/HARDWARE_VERIFICATION.md"
out="$here/release/hardware-kit"
rm -rf "$out"; mkdir -p "$out"
n=0; missing=0
grep -E '^\| [0-9]+ \| `[a-z0-9_/]+` \|' "$doc" | while IFS= read -r line; do
    num=$(printf '%s' "$line" | sed -E 's/^\| ([0-9]+) \|.*/\1/')
    ex=$(printf '%s' "$line" | sed -E 's/^\| [0-9]+ \| `([a-z0-9_/]+)`.*/\1/')
    rom=$(ls "$here/examples/$ex"/*.sfc 2>/dev/null | head -1)
    if [ -z "$rom" ]; then
        echo "MISSING $ex (no .sfc in examples/$ex) — build it first (make examples)" >&2
        touch "$out/.missing"
        continue
    fi
    dst=$(printf '%02d_%s.sfc' "$num" "$(printf '%s' "$ex" | tr '/' '_')")
    cp "$rom" "$out/$dst"
    echo "$dst"
done
if [ -e "$out/.missing" ]; then rm -f "$out/.missing"; exit 1; fi
sed -n '/^Session:/,/^22 /p' "$doc" > "$out/GRID.txt"
echo "hardware-kit: $(ls "$out"/*.sfc | wc -l) ROMs + GRID.txt in $out"
