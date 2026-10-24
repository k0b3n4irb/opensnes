# VRAM regions for fading — devtools/vram_layout. Regenerate:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/transitions/fading \
#     --pin bg_tiles=0x2000 --pin bg_map=0x1000
# The picture is 818 distinct tiles (13 088 words).
#
# name      kind     size_words
bg_tiles    bg_char  0x3400
bg_map      bg_map   0x400
