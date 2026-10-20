# Metatile table and VRAM per scene (agreed 2026-10-20, not started)

A second game built on the SDK (a vertical shoot 'em up, five levels, one
loaded at a time) reported three frictions on 2026-10-20. Two are taken, in
this shape, agreed with its session; the third stays on its side for now.
It is a port under licence like the first one: never named here.

## 1. `opensnes-tileset convert … --metatiles 16`

`--size 16` already exists but writes one entry per 16-px block (the PPU's
16x16 tile mode), not a metatile table. The game built its own sheet (one
metatile per 16-px block, in its original's order) and read the two-entry
wide `.map` as a table — a misuse that works.

Agreed output:

- `lvl.pic`: the distinct 8x8 tiles (flips folded with `--flip`);
- `lvl.mtl`: one row per distinct metatile, four 16-bit entries (top left,
  top right, bottom left, bottom right) in the map entry format;
- `lvl.mtm`: for each 16-px block of the picture, the metatile it became
  (u16, width/16 x height/16, no padding); two identical blocks share a
  number;
- `lvl.inc`: `lvl_metatiles`, `lvl_metamap` with their `_end`, and
  `#define LVL_METATILES n`.

The 1024-tile limit stays refused. Side point: the plain `.map` is padded to
2048 bytes (a 32x32 screen) even for a 16-px wide picture; decide whether to
document it or drop the padding outside `--pages`.

## 2. The `[ASSETS] VRAM` line of `opensnes-rom`

`report_assets()` (tools/opensnes-rom/src/checks.c) sums every `.pic` /
`.map` under the project: 298 % for five levels of which one is loaded. It
is an inventory, not a gate, but the percentage misleads. Agreed: when `res/`
has subfolders, one line per subfolder and "heaviest scene: X K / 64 K";
without, the total labelled "all assets, not necessarily loaded together".

This needs `make/common.mk` to find the `.toml` files in `res/*/` too (today
`*.toml` and `res/*.toml`): a change to common.mk, so in a separate worktree
(`exchanges.md`), and announced to both games before and after.

## 3. Sprite frame cache — on the game's side first

Up to 108 objects on screen, 8 KB of sprite tiles left, and the dynamic
engine does not fit (bases fixed at $0000/$1000, seven uploads a frame). The
game writes a cache of fixed slots (upload a frame only when it is missing)
on its real case, then reports what is general: the call, the measured cost
per frame, a reduced case of our own. Freeing the dynamic engine's fixed
bases is a receivable request if the cache shows it is needed.

## Status

Waiting for the likemario lot (a complete game, 2026-10-20) to land.
