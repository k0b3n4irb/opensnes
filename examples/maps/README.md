# Maps

This category contains examples that demonstrate tilemap-based engines and map collision systems on the SNES. On the SNES, "maps" refer to both the hardware tilemaps (the 32x32 or 64x64 grids of tile entries that the PPU renders as backgrounds) and the higher-level game maps built on top of them (scrolling worlds, collision grids, entity placement). These examples show two different approaches: a custom tilemap sprite engine that treats background tiles as movable objects, and a full platformer using the OpenSNES map and object engines with slope collision.

## Examples

### [dynamic_map](dynamic_map/)

**Custom Tilemap Sprite Engine** -- Demonstrates how to build a tile-based sprite system on BG1 using Mode 3 (8bpp, 256 colors). Supports both 32x32 and 64x64 map grids, with gargoyle sprites placed and scrolled interactively. Includes a C64-to-SNES sprite format converter. Teaches extended WRAM buffer management (bank $7E) and the 1-page-per-VBlank DMA pattern for safe tilemap transfers.

### [slopemario](slopemario/)

**Platformer with Slope Collision** -- A Mario-style platformer that uses `objCollidMapWithSlopes()` for diagonal terrain. Combines the map engine (scrolling tile world), the object engine (entity init/update callbacks), and the dynamic sprite engine (animated character rendering). Demonstrates player physics with acceleration, jumping, and slope-following, plus camera tracking via `mapUpdateCamera()`.

## Key SNES Concepts

### Hardware Tilemaps

The PPU reads tile indices from VRAM tilemap regions configured via registers $2107-$210A (BG1SC through BG4SC). Each tilemap entry is 2 bytes: a 10-bit tile number plus palette, priority, and horizontal/vertical flip bits. The PPU renders these tiles automatically every frame -- the CPU only needs to update the tilemap data in VRAM.

### Tilemap Sizes

Tilemaps can be 32x32 (SC_32x32, 2KB) up to 64x64 (SC_64x64, 8KB = four 32x32 pages). Larger tilemaps allow the hardware scroll registers ($210D-$2114) to pan across a bigger area without any CPU tile streaming. For worlds even larger than 64x64 tiles, software must stream new rows and columns into VRAM during VBlank.

### Tile-Based Collision

Instead of checking sprite-to-sprite bounding boxes, the game reads tile type data from ROM arrays at the player's position. Tile types encode properties like solid, empty, platform, slope angle, and hazard. The `objCollidMapWithSlopes()` function extends basic collision with diagonal surface support.

### VBlank DMA Budget

The SNES PPU ignores VRAM writes during active display. All tilemap and tile data updates must happen during VBlank (~41,000 master cycles available after NMI overhead). At 8 cycles per byte, a safe transfer limit is roughly 4-5KB per frame. Larger transfers require splitting across multiple VBlanks or using forced blank.

---

Study **dynamic_map** first for tilemap manipulation fundamentals, then **slopemario** for a complete platformer framework with slope collision.
