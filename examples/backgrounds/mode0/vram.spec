# VRAM regions for mode0 (4 BG layers) — devtools/vram_layout.
# Addresses pinned to the original layout (rendered identically); regenerate with:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/graphics/backgrounds/mode0 \
#     --pin bg0_tiles=0x2000 --pin bg1_tiles=0x3000 --pin bg2_tiles=0x4000 --pin bg3_tiles=0x5000 \
#     --pin bg0_map=0x0000 --pin bg1_map=0x0400 --pin bg2_map=0x0800 --pin bg3_map=0x0C00
# Validated at build by `make lint-vram`.
#
# name        kind     size_words
bg0_tiles     bg_char  0x1000
bg1_tiles     bg_char  0x1000
bg2_tiles     bg_char  0x1000
bg3_tiles     bg_char  0x1000
bg0_map       bg_map   0x400
bg1_map       bg_map   0x400
bg2_map       bg_map   0x400
bg3_map       bg_map   0x400
