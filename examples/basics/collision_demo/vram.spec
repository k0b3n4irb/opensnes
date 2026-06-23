# VRAM regions for collision_demo — consumed by devtools/vram_layout.
# Regenerate vram_map.h after edits:
#   python3 devtools/vram_layout/vram_layout.py --emit examples/basics/collision_demo
# Validated at build by `make lint-vram`.
#
# name        kind     size_words
bg_tiles      bg_char  0x10     # empty + wall, 2bpp (8 words each)
bg_tilemap    bg_map   0x400    # 32x32 tilemap (1024 words)
obj_tiles     obj      0x20     # player + enemy, 4bpp (16 words each)
