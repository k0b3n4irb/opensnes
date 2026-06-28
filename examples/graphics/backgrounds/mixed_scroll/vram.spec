# VRAM regions for mixed_scroll (BG1 + BG2) — devtools/vram_layout.
# Addresses pinned to the original layout (rendered identically); regenerate with:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/graphics/backgrounds/mixed_scroll \
#     --pin bg1_tiles=0x4000 --pin bg2_tiles=0x5000 --pin bg1_map=0x1800 --pin bg2_map=0x1400
# Validated at build by `make lint-vram`.
#
# name        kind     size_words
bg1_tiles     bg_char  0x1000
bg2_tiles     bg_char  0x1000
bg2_map       bg_map   0x400
bg1_map       bg_map   0x400
