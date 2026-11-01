# OpenSNES against PVSnesLib 4.6.0: the whole bench suite, measured again (2026-11-01)

Tree: `develop` at `537024cf` (examples rebuilt clean). PVSnesLib: tag **4.6.0**
(`1fab4b2a`, 2026-07-28, the newest tag of upstream as fetched today), compiler,
tools and library built from source on this machine. luna 1.40.0. Everything is
master cycles on luna (an NTSC frame is 357,370). Suites: `make bench` (static),
`bench-sdk` (23 workloads, same C on both), `bench-lib` (same library calls),
`bench-twins` (examples both SDKs have; ports, not the same source),
`bench-slots` (our two sprite engines). Raw output: the `b_*.log` files of the
session job directory, copied below where it matters.

Two tooling fixes were needed because 4.6.0 changed its `hello_world` example
(no more `data.asm`, `hdr.asm`, font in the example: generated or built into
the library): `devtools/sdkbench/run.py`, `devtools/libbench/run.py` and
`devtools/libbench/scene.c` (the PVSnesLib side uses `consoleInitDefaultText`).
The committed PVSnesLib references were refreshed to 4.6.0.

## 1. Where we win

### 23 identical workloads (`bench-sdk`): cycles −63.2 %, code size −52.3 %

| workload | PVSnesLib | OpenSNES | cycles | code |
|---|---:|---:|---:|---:|
| copy | 4,485,180 | 1,009,834 | −77.5 % | −57.4 % |
| physics | 10,031,454 | 2,272,996 | −77.3 % | −65.9 % |
| decimal | 8,787,672 | 2,243,748 | −74.5 % | −52.6 % |
| dist | 27,805,026 | 7,132,054 | −74.3 % | −56.0 % |
| sieve | 4,198,808 | 1,166,232 | −72.2 % | −63.9 % |
| tilemap | 2,916,926 | 842,314 | −71.1 % | −54.4 % |
| sort | 2,620,372 | 792,608 | −69.8 % | −54.4 % |
| depot | 37,799,876 | 11,868,668 | −68.6 % | −59.2 % |
| place | 14,371,286 | 4,828,896 | −66.4 % | −59.4 % |
| entities | 8,851,278 | 3,025,144 | −65.8 % | −55.5 % |
| switch | 2,697,292 | 940,658 | −65.1 % | −45.7 % |
| collide | 13,820,762 | 5,016,178 | −63.7 % | −59.2 % |
| grid | 5,262,636 | 2,029,842 | −61.4 % | −36.6 % |
| div32 | 11,361,922 | 4,497,130 | −60.4 % | −54.8 % |
| long | 5,944,006 | 2,385,376 | −59.9 % | −51.3 % |
| bytes | 7,862,558 | 3,186,932 | −59.5 % | −49.3 % |
| farfill | 134,396,874 | 55,196,686 | −58.9 % | −43.2 % |
| list | 2,858,638 | 1,324,896 | −53.7 % | −33.6 % |
| crc | 2,752,124 | 1,343,372 | −51.2 % | −53.6 % |
| strings | 3,082,148 | 1,547,512 | −49.8 % | −35.7 % |
| calls | 5,041,974 | 2,562,518 | −49.2 % | −45.7 % |
| state | 1,831,138 | 945,308 | −48.4 % | −31.1 % |
| mul | 6,389,950 | 3,343,258 | −47.7 % | −51.6 % |
| **total** | **325,169,900** | **119,502,160** | **−63.2 %** | **−52.3 %** |

OpenSNES is faster and smaller on all 23; deeper in stack on one (`grid`, 44
against 39 bytes). Last measurement (2026-10-18): −62 %.

### Library calls (`bench-lib`, mclk for one frame's worth)

| row | PVSnesLib | OpenSNES | |
|---|---:|---:|---:|
| text | 82,796 | 17,366 | −79.0 % |
| world (sprites placed from a world position) | 129,336 | 31,741 | −75.5 % |
| worldo | 136,697 | 36,311 | −73.4 % |
| worldc | 129,336 | 53,414 | −58.7 % |
| frame (a whole frame's calls) | 167,840 | 91,387 | −45.6 % |
| oamsize | 48,741 | 28,886 | −40.7 % |
| vramq (queued VRAM transfers) | 17,395 | 10,576 | −39.2 % |
| scroll | 39,544 | 27,356 | −30.8 % |
| oamset | 81,074 | 64,978 | −19.9 % |
| idle (empty handler) | 7,184 | 5,852 | −18.5 % |
| pad | 11,435 | 9,742 | −14.8 % |
| vramc | 17,395 | 15,916 | −8.5 % |
| oamxy | 45,059 | 42,475 | −5.7 % |
| dma | 17,899 | 17,866 | −0.2 % |

No row where we are behind. `dma` is a tie because the cost is the bus, not
the code; `oamxy` (−5.7 %) and `vramc` (−8.5 %) are the thin ones.

## 2. Where we lose or tie

`bench-twins` (examples both SDKs ship, run side by side with the same input,
mean master cycles of work per frame). The twins are **ports, not one source**,
so a gap is a lead to read, not a verdict:

| example | PVSnesLib | OpenSNES | |
|---|---:|---:|---:|
| rooftop_run (a game vs upstream's game) | 55,402 | 172,214 | **+210.8 %** |
| controller | 71,083 | 158,614 | **+123.1 %** |
| dynamicmetasprite | 44,148 | 55,048 | **+24.7 %** |
| slopemario | 58,282 | 68,972 | **+18.3 %** |
| mapandobjects | 64,057 | 68,972 | +7.7 % |
| mapscroll | 18,603 | 19,251 | +3.5 % |
| hello, simplesprite, mode1, gradient, objectsize | 7.5–8.2 k | 6.2–6.8 k | −16 to −20 % |
| continuousscroll, mode7perspective, parallax | | | −26 to −30 % |

What the profiles say (top rows of `bench-twins DETAIL=1`):
- **`oamHide` is the single hottest routine** in two of the three worst twins:
  22,148 mclk a frame in rooftop_run, 22,592 in controller (about 175 mclk per
  call, 128 calls). PVSnesLib's equivalents do not appear in its top five.
- rooftop_run adds an inlined per-object loop (`shards_update`, 15,474 + 12,478)
  that is game logic, not SDK overhead: the pairing is unfair there, but the
  loop's own cost is ours to cut (see `eeb6616e`).
- dynamicmetasprite: `oamMetaDrawDyn` 13,018 + `_o16d_attr` 4,852 +
  `_o16d_store_tile` 4,352 against PVSnesLib's `_oMTDD*` family: the dynamic
  metasprite draw is slower than theirs for the same picture.
- slopemario/mapandobjects: the object engines cost the same on both sides
  (`_oiual2` 8.8 k, `_oiual10` 7.7 k both); the extra 10 k are in our NMI
  handler (`NmiHandler@oam_done` 4.6 k) and a map load routine (`_stw_load2`
  4.1 k).

Regressions on our own side since the last measurement (`bench-sdk` baseline
of 2026-10-18; PVSnesLib's figures moved by under 0.3 %, its compiler is
unchanged from 4.5): **tilemap +3.6 %, switch +3.2 %, physics +3.1 %,
sieve +2.6 %, grid +1.6 %, entities +1.1 %, long +1.3 %**. Against that,
mul −23.3 %, strings −10.8 %, depot −8.3 %, place −6.6 %, collide −4.3 %: the
total went from 122.72 M to 119.50 M (−2.6 %). The CI gate (+5 % total,
+25 % per workload) saw none of the seven. They are a drift, not an incident,
and nobody has bisected them.

## 3. Static estimate (`make bench`, 34 functions)

1,234 cycles against 1,238 at baseline (PVSnesLib+opt 1,981: −37.7 %).
PVSnesLib+opt still wins `pea_constant_args` and `mod_const_10`; pointer-heavy
code is where the 4-byte pointer ABI costs us (`docs/BENCHMARK.md`).

## 4. Our sprite engines (`bench-slots`, NTSC, mean mclk a frame)

| variant/scene | work | writes lost outside blanking |
|---|---:|---|
| dynamic engine, 16 px, mix | 70.8 k (decide+draw+NMI+flush) | **22,016 B of VRAM, 76,160 OAM** |
| fixed slots, 16 px, mix | 87.1 k | 0 |
| dynamic, 16 px, burst | 52.2 k | 4,096 B VRAM lost |
| fixed slots, 16 px, burst | 63.2 k | 0 |

The dynamic engine is 19 % cheaper and **drops transfers** when the scene
bursts; fixed slots are slower and lose nothing. Neither beats the other
outright, which is why the module is still open.

## 5. What counts for a game

A game's frame is 357 k mclk and 224-line VBlank offers ~50 k. Three things
decide whether a game fits, in this order:
1. **VBlank DMA and OAM**: `oamHide` loops, `oamSet` (2,031 mclk a call here,
   2,534 theirs) and the NMI handler are paid every frame.
2. **Object and sprite loops**: the compiler's lead (−63 %) is what pays for
   them; a −3 % drift on a loop is 3 % of the logic budget.
3. Everything else (tilemap, text, scroll) is already 30–80 % under.

On the code a user writes, we are faster and smaller everywhere we measure.
On the bits of the library that are called every frame in every game
(sprite hiding, dynamic metasprites, the NMI), the picture is mixed.

## 6. What to concentrate on, by gain

| # | Work | Gain | Cost |
|---|---|---|---|
| 1 | `oamHide` per call ≈ 175 mclk: add a bulk `oamHideRange(first, count)` (word stores) and make `oamInit`/clears use it | ~15–18 k mclk a frame on any scene that clears or hides in a loop (≈5 % of a frame; it is the top line of two twins) | small: one lib asm routine + test, about a day (Class B) |
| 2 | Dynamic metasprite draw: `oamMetaDrawDyn` + `_o16d_*` ≈ 22 k a frame, 25 % over PVSnesLib | ~8–10 k a frame | medium: restructure the draw loop (compiler help from the per-object-loop work), 2–3 days |
| 3 | Bisect the +1 to +3.6 % drift on seven workloads | back ~3 % of the loop code | small per workload: `git bisect` over `devtools/sdkbench` (a minute a step), about a day; a per-workload gate at +2 % would have caught it |
| 4 | Dynamic engine losing VRAM/OAM transfers on bursts (`vram_lost` 22 kB) vs fixed slots | correctness on busy scenes, not cycles | medium–large: the module planned from slotbench; decide how it degrades |
| 5 | NMI handler and map-load tail on object scenes (`oam_done` 4.6 k, `_stw_load2` 4.1 k) | ~5–8 k a frame on map+object games | medium, needs the NMI audit rules (`nmi_audit.md`) |
| 6 | `oamSet` 2,031 mclk a call and `oamXY` −5.7 %: smallest margins in `bench-lib` | ~10 % of the sprite budget | medium: the call is already hand-tuned, little left without an inlining option |

## 7. What this suite does not cover

No audio cost against PVSnesLib (SNESMOD vs its driver), no PAL run, no
compile-time or ROM-size-with-assets comparison, and the twins are ports.
The suite is also blind to the CI gate's loose thresholds (point 3 above).

## 8. The drift of seven workloads, bisected (lot 2, 2026-11-01)

Method: a separate worktree, every commit between `2289f30b` (baseline of
2026-10-18) and `0342f046` that touches `compiler/`, `lib/`, `templates/`,
`make/` or `runtime/` (32 of them), `make clean && make compiler tools lib`,
then `devtools/sdkbench/run.py` on the seven workloads. Clean builds take ten
seconds; the whole scan took a few minutes. Use the pinned luna
(`testing/bin/luna`): a worktree without it falls back to the luna on `PATH`,
which attributed the cycles differently and showed no drift at all.

| step | commit | effect |
|---|---|---|
| 1 | `04d907fb` nmiSetLag | +0.03 % everywhere (a few hundred cycles) |
| 2 | `a061a2db` code in the `$80` mirror | **+28.8 k on every workload** (+0.9 % to +3.5 %): a measuring artefact, fixed |
| 3 | `2f14bdd6` `__mul16` skips empty partial products | physics +4.2 k, grid +16.8 k, entities +4.2 k: a branch paid on operands whose high byte is not 0 |
| 4 | `f7f11dfe` short path in `tcc_mul32` | long +30.3 k (+1.3 %): twelve cycles of test on every full 32-bit product |
| 5 | `eeb6616e` per-object loop | physics +35.6 k (+1.6 %), grid −14.3 k: one `tax` became `ldx.b` (A was no longer known across the loop test) |

**Step 2 is not a cost.** Every NMI now enters through `NmiEntry` in bank `$00`
(`jml.l NmiHandler`, 32 master cycles a frame, 28,800 over 900 frames). The
vector label sat at `$80:xxxx` while the program counter was `$00:xxxx`, so
luna folded those cycles onto whatever symbol came before in bank `$00`:
`_reserved_7e_mirror` in the baseline ROM, `tmap.workloads` in the workload ROM.
The two did not cancel and each workload paid the NMI jump once more. `NmiEntry`
is now a section of its own addressed at `BASE $00`, the profile counts it as
`NmiEntry` in both ROMs and it cancels. The real cost is 32 mclk a frame (0.009 %
of a frame) and it buys FastROM for the whole C code and library.

**Steps 3 to 5 are kept.** Each is a data-dependent fast path or a heuristic
that wins elsewhere (`mul` −23 %, `strings` −11 %, `grid` ends −1.6 % against
10-18) and costs 0.1 to 1.8 % where the operands are always wide. Undoing them
would lose more than it gives. Step 5 is the one with a lead: the loop test
`lda S / cmp / bcc` leaves A = S, and the peephole no longer knows it at the
target of the branch (`ldx.b S` instead of `tax`, +2 cycles an iteration); it is
a Class A change for 1.6 % of one workload, not taken.

After the fix the seven stand at (against 10-18): sieve +0.04 %, physics +1.8 %,
long +1.3 %, switch +0.04 %, tilemap +0.05 %, grid +0.16 %, entities +0.17 %.

**Gate.** `devtools/sdkbench/run.py --check` fails at **+2 %** in total and on any
one workload (it was +5 % and +25 %). Baseline re-measured: total 119,128,592.

## 9. Dynamic metasprite draw (lot 3, 2026-11-01)

Row 2 of §6 done in the draw routines rather than in the iterator: the
`_o16d_*` / `_o8d_*` tails did a `php`/`plp` round trip for the carry, two
read-modify-writes of the high table through other tables, and three tests of
the same size. One macro (`DYN_PACK_TAIL`, `sprite_dynamic.asm`) now does the
position, the attribute and the two high-table bits in one pass; `oammask`
carries what it needs. Scene `sprites/dynamic_sprite` (SELECT), busy mclk a
frame, luna 1.40: **55,048 to 46,360 (-15.8 %)**; PVSnesLib's twin was 44,332
on 2026-10-09 (the twin's ROM was not rebuilt here: re-measure with
`make bench-twins` before quoting the gap). The iterator
(`oamMetaDrawDyn@item`, 13 k) would gain about 7 cycles an item from a running
offset: not taken.
