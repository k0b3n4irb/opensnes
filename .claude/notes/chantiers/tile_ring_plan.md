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

## Status (2026-11-20, Opus agent in its own worktree, to be re-validated by the session)
Steps 1-3 written: `opensnes-tilering` (tool + golden suite), module `tilering` (`tilering.c` +
`tilering_cells.asm`), `examples/scrolling/tile_ring` (1,433 characters, ring of 640, 1,052-frame course),
`testing/tile_ring_budget.py` (in `make tests`) and three manifests. Measured on luna v1.44.0:
- flush worst frame: staged 7,994 mclk, direct 19,270; + NMI 6,216 = 28 % / 51 % of the NTSC blank
  (50,468), 12 % / 21 % of PAL (118,668); 21 new characters at most a frame on the course, 5 / 22 entries.
- CPU outside the blank: C cell loop 4,500 mclk a cell (318 lag frames on the course) -> assembly
  ~1,000 a cell; worst frame 276,536 mclk (77 % of NTSC), none lagging.
- Computed bound (vramqueue model, not measured): the converter's worst diagonal, 49 characters, is
  ~17 k mclk staged (6 entries) but ~47 k direct (53 entries): direct mode can overrun NTSC there.
- Yardsticks (`tech/commercial_yardsticks.md`): E2 builds a tilemap entry in 256-274 mclk and its worst
  diagonal in 39,002 (8 transfers, 264 bytes in the blank). Ours does more per cell (a slot lookup and a
  count on the way in, another on the way out, no character streaming in E2) and costs ~1,000 mclk a cell
  in, ~500 out; the worst diagonal with 40+ character loads is 7x E2's. In the blank we are close: 5
  entries and ~920 bytes staged on the course. Next lever if a game needs it: a RAM mirror of the
  window's characters so the leaving column is read from RAM, not the map, and in/out in one pass.
Open: a re-centre policy stays the game's; map parts > 1 exercised by the example (2 map parts).
