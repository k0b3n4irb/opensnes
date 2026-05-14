# LikeMario Port — Lessons Learned

## Input System Architecture
- **`padUpdate()` in input.c is a NO-OP** — exists only for API compatibility
- Input is read entirely by NMI handler in crt0.asm into `pad_keys`/`pad_keysdown`
- `padHeld()` reads `pad_keys[]`, `padPressed()` reads `pad_keysdown[]` — both NMI-populated
- NMI edge detection: `pad_keysdown = (new ^ old) & new` — returns 0 when no buttons pressed,
  even with garbage in `pad_keysold` on first frame (because 0 & anything = 0)
- **Always read the ACTUAL linked source** — symbol map labels like `@if_true.5` indicate
  compiled C, not hand-written assembly. Check `lib/source/*.c` not `libsnes.asm`

## Scroll Engine — Off-by-One Fix
- **Stream column at `tile_x + 32`, NOT `tile_x + 31`**
- With sub-tile scrolling, the visible area spans 33 tile columns (32 full + 1 partial)
- At `tile_x + 31`, the 33rd partial tile at the right edge has stale VRAM data
- Symptom: rightmost 1-8 pixels show wrong tiles on first scroll, self-corrects later
- PVSnesLib uses `MAP_DISPW + 1 = 33` columns for the same reason

## Map Data (BG1.m16 / map_1_1.b16)
- **Header**: 3 u16 words (width_px, height_px, unknown), then tile data
- **Tile property table** (tilesetatt/map_1_1.b16): 48 u16 entries, index = tile number & 0x3FF
- Tile 0 = 0xFF00 (SOLID) — used for borders and underground
- Tile 1 = 0x0000 (EMPTY) — used for sky/air in playable area
- Tile 40 = 0xFF00 (SOLID) — ground surface at row 14 (y=112)
- Ground level for Mario: y=96 (feet at y=112 = row 14 boundary)

## VRAM Layout (SC_64x32)
- Two 32x32 nametable pages: page 0 at VRAM_BG_MAP, page 1 at VRAM_BG_MAP+$0400
- Column write with VMAIN=$81 (increment by 32 on high write) stays within page bounds
- Initial load: 64 columns during force blank covers entire VRAM tilemap
- Runtime streaming: 1 column per frame via RAM buffer (prepare during active, flush in VBlank)

## Pointer Array Stride
- QBE w65816 backend uses 8-byte stride for pointer arrays (`u16 *arr[]`)
- Indexing: `asl a; asl a; asl a` (× 8), but only 4 bytes written (low word + high word = 0)
- Read side uses same stride, so it's consistent — just wastes 4 bytes per entry

## Debugging Process Notes
- Verified all assembly generation was correct (register writes in 8-bit mode, pointer
  arithmetic scales by sizeof(u16), scroll register write-twice protocol)
- Used `xxd` on ROM binary to read map header and tile properties directly
- Python script for map data analysis (tile frequencies, level layout) was very effective
- Stack depth analysis: worst chain main→collide_vertical→map_get_tile_prop ≈ 210 bytes (safe)
