# VRAM regions for metasprite — consumed by devtools/vram_layout.
# Regenerate vram_map.h after edits (sprites pinned low: OBJSEL base is hardcoded 0):
#   python3 devtools/vram_layout/vram_layout.py --emit examples/graphics/sprites/metasprite --keep-low sprites
# Validated at build by `make lint-vram`.
#
# The sprite sheet holds three sub-sheets (hero32/16/8) the example lays out
# internally at fixed offsets from its base; we model it as ONE obj region that
# reserves BOTH 8 KB name pages (name_select puts page 1 at base+0x1000, and
# hero8 lives there). OBJSEL is programmed with OBJ_SIZE_TO_REG alone (name base
# = 0), so this region must stay at 0x0000 — hence --keep-low.
#
# name        kind     size_words
sprites       obj      0x2000   # hero32(0xC00)+hero16(0x600)+hero8(0x100) across 2 name pages
font          bg_char  0x600    # opensnes_font_4bpp = 3072 bytes = 1536 words
text_map      bg_map   0x400    # 32x32 tilemap
