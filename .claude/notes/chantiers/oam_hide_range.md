# oamHideRange and oamHide in assembly (2026-11-01)

Status: done, first of three lots decided after the PVSnesLib 4.6.0
benchmark (`.claude/notes/reviews/2026-11-01_benchmark_pvsneslib_4_6_0.md`).
Next, one conversation each: (2) bisect the 1 to 3.6 % drift of seven
`sdkbench` workloads since 2026-10-18, then a per-workload CI gate at +2 %;
(3) the dynamic metasprite draw (`oamMetaDrawDyn`, `_o16d_*`).

## What was measured before

`devtools/libbench` gained two rows: `hideeach` (32 `oamHide()` calls) and
`hiderange` (one range call here, `oamClear(first, count)` there).
Before: `oamHide()` 1,790 mclk a call, 57,271 mclk for 32; PVSnesLib's
equivalent 1,443 a call. The C `oamHide()` was the top line of two twin
examples (22 k a frame).

## Done

- `oamHideRange(first, count)` and `oamHide(id)` in
  `lib/source/sprite_oamset.asm` (the low table is one word a sprite, y 240
  over x 1; the high table is filled a byte of four sprites at a time, sprite
  by sprite at the two edges). Clamped at 128; `oam_max_id` and
  `oam_update_flag` as `oamHide()` always did.
- After: `oamHide()` 800 mclk (-55 %), 32 sprites in one call 9,824 mclk
  (-83 %, 5.8 times cheaper); `bench-lib` no longer lists a row where we are
  behind.
- Test: `testing/fixtures/libtests` (`r_hrange`): nine cases, the range and
  `oamHide()` each byte for byte against the definition written out in C,
  edges inside a byte of the high table, whole bytes, the clamp, an id past
  the end, a count of 0, and `oam_max_id`. The fixture's `STEPS` went from
  4 M to 5 M for the extra loops.

## Left

- `oamClear()` (hides all and zeroes tile and attributes) is still C; it is
  not a per-frame call.
- `oamSet` at 2,031 mclk a call is still the heaviest per-sprite call.
