#!/usr/bin/env bash
# Reproduce the split of Kenney's "Pixel Shmup" tiles_packed.png into the two
# inputs gfx4snes consumes. The original packed sheet bundles sprite/HUD tiles
# (top 3 rows) with terrain tiles (bottom 7 rows); SNES wants them on distinct
# palettes (sprites in CGRAM 128+, BG in CGRAM 0+), so we split before import.
#
# Re-run this script if tiles_packed.png is replaced with a newer Kenney drop.
#
# Source pack: https://kenney.nl/assets/pixel-shmup   (CC0)
# Inputs : tiles_packed.png (192x160, 23 unique colors)
# Outputs: sprites.png      (192x48, 14 colors — top 3 rows, transparent bg)
#          ground.png       (192x112, 18 colors — bottom 7 rows, opaque)
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$here"

if [ ! -f tiles_packed.png ]; then
    echo "tiles_packed.png missing — fetch from https://kenney.nl/assets/pixel-shmup"
    exit 1
fi

magick tiles_packed.png -crop 192x48+0+0   +repage sprites.png
magick tiles_packed.png -crop 192x112+0+48 +repage ground.png

echo "Split outputs:"
for f in sprites.png ground.png; do
    identify -format "  %f : %wx%h  unique=%k\n" "$f"
done
