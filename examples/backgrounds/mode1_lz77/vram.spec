# VRAM regions for mode1_lz77 — devtools/vram_layout. Addresses pinned (identical
# render). Regenerate:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/graphics/backgrounds/mode1_lz77 \
#     --pin bg_tiles=0x4000 --pin bg_map=0x0000
#
# name      kind     size_words
bg_tiles    bg_char  0x1000
bg_map      bg_map   0x400
