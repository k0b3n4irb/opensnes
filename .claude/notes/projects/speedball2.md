# Speedball 2 on SNES — the first real game on the SDK

Owner's project, local only (`~/workspace/speedball2-snes`; the rights
belong to Rebellion, nothing leaves the machine). Since 2026-10-10 it
builds against THIS working tree (`../opensnes`, branch `develop`): what is
in `bin/` here, committed or not, is what its `make` uses. A session of its
own works there with its own `CLAUDE.md`; from here we read it, and we
build and measure copies of `game/` in the scratchpad
(`make OPENSNES=<this tree> [CC=<another bin>/cc65816]`). Owner,
2026-10-10: it is our first game, not an example — use it to perfect the
SDK and to challenge the port's approach.

## How to measure a compiler or library step on it

Its `game/test/play.toml` input script, then
`luna profile game.sfc --from-frame 0 --until-frame 760 --input <script> --top 3000`
and the sum of master cycles of the symbols of `match.c` (`*.match`,
`match*`). NOT the busy share of a frame window: the game computes a tick
straight through and catches up, so a faster build does more work in a
given window. Reference points (2026-10-10): 99.0 M at `a66a4f53`, 98.6 M
with whole-function inlining, 93.6 M with direct-page temps in functions
that call (92.1 M with `inline` on `vectorLength` and `octantTo`).

Its two tests (`boot`, `play`) hash pictures at fixed frames and press the
pad at fixed frames while the game advances by tick: every step that makes
the code faster changes them. `luna diff a.sfc b.sfc --tolerance 6` with no
input and `testing/frame_sequence.py` tell "earlier" from "wrong".

## What it has shown the SDK

- The time is in the bodies, not in the calls; a leaf poured into a
  function that calls loses; 59 % of the logic was on stack frames.
- `audioLoadSample` raced on its end mark (found the same day by our own
  library test, once the loader's loop got faster).
- The map engine could not host its pitch: `MAP_MAXMTILES` 512 distinct
  tiles where it needs over 1000, `tmx2snes` 64x64 where it has 80x144. It
  wrote its own ring-tilemap module (`pitch.c`). A real gap, not planned yet.
- What a lag frame guarantees was written nowhere (now in the frame-budget
  page).
- `spritesUpdate` is its heaviest function outside the logic: #167
  (`order` in `OamWorldBatch`) would take part of it.
- The SA-1 is ruled out there because C does not run on it. The AI of 18
  players is the textbook load for it: a chantier to weigh.

## What we put to the owner about the port (2026-10-10)

1. The open question of decision 0003 — does the US version keep the PAL
   speed constants? — decides whether 30 ticks a second is the original's
   pace or 20 % above it, and the whole CPU budget hangs on that pace.
2. "Never a three-frame tick" may be a harder criterion than needed: 7 of
   290 today, about 29.6 ticks a second.
3. Tests indexed by frame move with the compiler; inputs indexed by tick
   (the original has a replay) would move only with the logic.
4. `buildColMap` copies the map by columns into 23 KB of RAM at boot, 38
   frames: a table the asset tool can produce, left in ROM.
5. "Kick-off under 40 %" no longer compares two builds (see above).
