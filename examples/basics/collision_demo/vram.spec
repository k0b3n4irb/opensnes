# VRAM regions for collision_demo — consumed by devtools/vram_layout.
# Regenerate vram_map.h after edits:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/basics/collision_demo
# Validated at build by `make lint-vram`.
#
# name        kind     size_words
bg_tiles      bg_char  0x100    # the four 16x16 cells, 16 tiles 4bpp (16 words each)
bg_tilemap    bg_map   0x400    # 32x32 tilemap (1024 words)
obj_tiles     obj      0x1400   # Kit's 15 frames and the drone's 4, 32x32: 320 tiles 4bpp
