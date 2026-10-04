---
name: A6+A7 unit/collision + unit/text residuals — autonomous diagnostic via mesen2-rpc
description: First chantier task driven entirely through mesen2-rpc. Initial hypothesis (ABI stack imbalance) was a stale-build red herring. Real residuals: 7 named test failures in rectInit/rectSetPos/point_inside/point_edge + 1 string corruption ("partial_ovZR00p"). 10 minutes of autonomous investigation.
type: project
---

# A6+A7 residuals — mesen2-rpc autonomous diagnostic

**Date**: 2026-05-14
**Driven by**: Claude alone via direct Mesen2Client TS calls (MCP server connection
had dropped due to earlier kill; the standalone path is identical
plumbing minus the stdio wrapper).
**No human in the diagnostic loop.**

## Real residuals on `wip/a6-a7-atomic-v3`

Quick suite: **267/269 passing**.

Two failures left, both runtime, both with named sub-assertions identified:

### `unit/collision` — 8/26 sub-tests fail

Read directly from `tilemapBuffer @ WRAM $0520` via mesen2-rpc after
60 frames. The text buffer is byte-indexed; each entry's low byte is
`tile_idx = ASCII - 32` so we recover full screen content without
needing OCR or VRAM tilemap reads.

```
FAIL: point_inside        ← collidePoint(110, 110, &r)  where r=(100,100,32,32)
FAIL: point_edge          ← collidePoint(100, 100, &r)  (boundary inclusion)
FAIL: rectInit_x          ← rectInit() writes the wrong field
FAIL: rectInit_y          ← (or all four fail together if the write loop is off)
FAIL: rectInit_w
FAIL: rectInit_h
FAIL: rectSetPos          ← same family — struct field write
```

Plus one **corrupted string** on line 3 of the screen:
```
PASS: partial_ovZR00p     ← should read "PASS: partial_overlap"
```

That's a buffer-write corruption — likely the same root cause as the
rect failures (a wrong struct/field offset is overwriting nearby memory).

### `unit/text` — 2 sub-tests fail

Not yet enumerated. Same diagnostic shape: load `test_text.sfc`, run
60 frames, dump the tilemapBuffer, read the FAIL lines.

## What MIS-LED my first probe

Before rebuilding the fixture, the `test_collision.sfc` was from
**April 27** — pre-A6 retrofit. The .c.asm pushed only 2 bytes for
each pointer arg (`pea.w string.N`) instead of the post-A6 4-byte
form (`pea.w :string.N` + `pea.w string.N`). That stale ABI caused
`textPrintAt` to read garbage for the msg pointer's bank byte, which
corrupted arbitrary memory, which eventually rolled SP into MMIO
space ($4212) and PC into bank $FF.

My first diagnostic probe read PC=$FF22 K=$FF SP=$4212 and (correctly
for the data I had) concluded "ABI stack imbalance". The hypothesis
was structurally right BUT the bug-already-fixed path was the answer:
**the compiler is correct, only the fixture's pre-compiled .c.asm was
stale**.

A `make clean && make` on the fixture rebuilds against the current
QBE codegen → ABI pushes are correct (verified by grep on the new
.c.asm: both `pea.w :string.N` and `pea.w string.N` present). Re-running
the suite then dropped failures from 12 → 8 in collision. Those
remaining 8 are real bugs, not build artifacts.

## What this proves about mesen2-rpc

Lessons hardened by this session:

1. **Always rebuild the fixture before drawing conclusions.** Stale
   pre-compiled .c.asm from a previous chantier baseline will lie to
   you. Fixture Makefiles should ideally have a sanity dependency on
   the cproc/qbe binaries — they don't yet. Worth a small chantier.

2. **`tilemapBuffer @ WRAM $0520` is the right thing to read for
   diagnostic output**, not VRAM. Text writes go through the RAM
   buffer; VRAM only updates on NMI DMA. Reading WRAM gives the
   authoritative "what did the program write" answer.

3. **Reading screen content via mesen2-rpc is fast**: 28 rows ×
   readRange(64) = 28 RPC calls = ~140ms. Identifying named
   sub-assertions this way is now trivial — was infeasible with the
   manual Mesen2 GUI workflow.

4. **The naive "PC + SP + counter dump" probe is the right opening
   move**: it caught the stale-build issue immediately (impossible
   counter values 32+34=66 out of 28 tests = something is very wrong).
   That's a 12-RPC-call test that should be templated for any future
   unit-test diagnostic.

## Concrete fix targets (next session)

For the 7 named failures + 1 string corruption:

1. **Build a minimal repro for `rectInit`**: a 4-line test that calls
   `rectInit(&r, 1, 2, 3, 4)` and reads back `r.x`, `r.y`, `r.w`, `r.h`.
   If they don't match → struct field write codegen is broken in
   `lib/source/collision.c`'s `rectInit`. Use mesen2-rpc to read
   the actual Rect bytes from WRAM after the call.

2. **`point_inside` and `point_edge` failures**: these are 0/1 boundary
   tests on `collidePoint`. They test `x == rect.x` and `y == rect.y`
   inclusive edges. Likely a u8/u16 signedness issue at the edge.
   Generate the test ASM for `collidePoint`, look for `bne`/`beq`
   that should be `bcc`/`bcs` (or vice versa).

3. **The string corruption "partial_ovZR00p"**: bytes at offsets 9-12
   of the string changed from "erlap" to "ZR00p". This is a memory
   write past end of buffer somewhere. Could be unrelated to the
   collision bugs OR could be the rectInit bug stomping on string
   storage. Worth resolving rectInit first and re-running.

## Diagnostic scripts to keep around

- `/tmp/diag_probe.mjs` — PC + SP + counter quick check
- `/tmp/diag_collision_fail_list.mjs` — tilemapBuffer screen dump
  with ASCII decoding (tile_idx = ASCII - 32 for OpenSNES SDK font)

Both should be moved into `tools/opensnes-emu/diag/` and parameterized
on ROM path + counter symbol names, so they're reusable for any unit
test fixture going forward.

## Comparison: workflow before vs after

Pre-2026-05-14 workflow for the same diagnosis:
- "K0b3, ouvre test_collision.sfc dans Mesen2"
- "K0b3, run 1 second"
- "K0b3, ouvre VRAM viewer, regarde tile map à 7000"
- "K0b3, c'est lisible ? Lis-moi les lignes FAIL:"
- "K0b3, OK fait défiler"
- Total: 20-30 minutes of K0b3 keyboard time per test fixture, MORE if
  the screen is gibberish and we have to dig into RAM directly.

Post-mesen2-rpc workflow (this session):
- `node /tmp/diag_probe.mjs` → counters + PC/SP in 300ms
- `node /tmp/diag_collision_fail_list.mjs` → full screen + FAIL list
  in 140ms
- **Total: ~10 minutes end-to-end including a rebuild that fixed 4
  of the 12 originally-reported failures.** Zero K0b3 keyboard time.

## Cross-references

- [[mesen2_rpc_mvp_validated]] — the tool's validation criterion
- [[a6_a7_unified_audit]] — chantier scope
- A6.12 commit — the 3 ASM sites already retrofitted
- `lib/source/collision.c` — where the 7 named failures live
- `tools/opensnes-emu/test/fixtures/unit/collision/test_collision.c` —
  the test source with TEST() macros
