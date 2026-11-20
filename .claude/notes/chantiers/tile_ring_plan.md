# Tile ring: 8-direction scrolling over a map whose characters exceed VRAM (plan, 2026-11-20)

## Why the lot reopens
Rule of two games (`partners.md`, check 1). On 2026-11-11 a second game measured its maps at 3/4 width
(`reduction34`): whole map 1,200 / 1,211 / 1,115 unique 8x8 characters (mirrors merged), tile sheets 1,085 to 1,531,
worst screen window 270. Over the 1,024 numbers of a tilemap entry in all cases, so a character set per level or per
map no longer fits. The first game measured 1,646 whole map, 448 worst window. Need is general.

## Mechanism (module `tilering`, optional, nothing in the kernel)
- Input: a deduplicated character table (mirrors merged, offline tool) and a map of 16-bit character numbers with
  attributes. Order of the table: first appearance row by row from the bottom of the map (locality of the ring).
- A ring of `N` character slots in VRAM (budget given by the caller, 512 to 768). Per slot: reference count or age.
- Tilemap: a row and a column built at the border (as `mapUpdate`), new characters pushed through `vramqueue`.
- Slots of a new row/column allocated in sequence, so the flush is 1 or 2 entries (measured: contiguous 57
  characters + tilemap row and column = 18,050 mclk, scattered 46,850 mclk, over the NTSC VBlank).
- Policy with the game: the module gives the mechanism and `tileRingCost()` (worst new characters per frame for a speed,
  against the DMA budget, PAL and NTSC); it never imposes a budget.

## Sizing from the partners
1 px per frame per axis, margin 2 px; worst per step 7 px on an axis (NTSC), 5 + 5 diagonal: at most one new row and
one new column of characters per step. Re-centre (camera jump, about 448 characters) is the game's policy, spread
over several VBlanks, with `tileRingFill()` for the screen-off start (14,336 bytes).

## Steps
1. Reproducer example `examples/scrolling/tile_ring` with an invented map larger than 1,024 unique characters.
2. Conversion tool: dedup with H/V mirrors, first-appearance order, 16-bit map.
3. Library module, then a luna test: worst frame count against the budget, PAL and NTSC, diagonal included.
4. Figures against the yardsticks of `tech/commercial_yardsticks.md` in the commit; doc and CHANGELOG.
Class: new lib module and tool, Class B. Written by an Opus agent in its own worktree, every gate re-run by the session.
