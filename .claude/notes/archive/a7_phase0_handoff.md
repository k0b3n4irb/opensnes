---
name: Chantier A7 Phase 0 handoff (RED proof + Phase 1 entry plan)
description: A7 (32-bit Kl codegen) Phase 0 complete on 2026-05-10. RED proof captured in compile-time evidence on 5/5 cases. Phase 1 entry plan documented. Repro source preserved here for /tmp survival.
type: project
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
A7 Phase 0 was completed on **2026-05-10** in the post-v0.17.0 session.
Phase 1 has NOT started — the scope review during initial reading showed
Phase 1 is properly 2-3 days of focused compiler work touching
`compiler/qbe/w65816/{abi,emit}.c` and at risk of cascading into the 30
existing QBE patches.

**Why**: closes a 🔴 silent corruption shipped in lib (`fixDiv` returns
`0` for any non-trivial fixed-point dividend); closes catalogue A7;
unblocks B5 fixed32.

**How to apply**: when reopening A7, start by reading this entry,
re-create `/tmp/a7_repro/` from the source below, rebuild, confirm 5/5
BUG still reproducible, then proceed with Phase 1.

## Phase 0 evidence (file:line, captured 2026-05-10)

5/5 BUG patterns confirmed at compile-time (no runtime needed to prove
the bug exists):

| # | Case | Pattern | Evidence |
|---|---|---|---|
| 1 | `add32(0xFFFF, 1)` | single `clc; adc` on low 16, no high-half | `/tmp/a7_repro/main.c.asm:13-21` |
| 2 | `mul32(0x100, 0x100)` | lowers to `jsl __mul16` (16×16→16) | `/tmp/a7_repro/main.c.asm:36-48` |
| 3 | `shr32(0x12345678, 16)` | variable-shift loop on single A reg | `/tmp/a7_repro/main.c.asm:109-138` |
| 4 | `shl32(0x18000, 1)` | idem | `/tmp/a7_repro/main.c.asm:64-93` |
| 5 | `fixDiv(FIX(1), FIX(5))` | `xba; and.w #$FF00; jsl __sdiv16` (matches catalogue verbatim) | `lib/build/lorom/math.c.asm:342-363` |

The catalogue claim at STRUCTURAL_DEFECTS.md:855-870 reproduces verbatim.

## Phase 0 reproduction source (to recreate /tmp/a7_repro/main.c)

```c
/**
 * @file main.c
 * @brief Chantier A7 reproduction — QBE 32-bit (Kl) codegen
 */
#include <snes.h>
#include <snes/math.h>
#include <snes/console.h>
#include <snes/text.h>

static u8 tests_passed;
static u8 tests_failed;

#define TEST(name, cond) do { \
    if (cond) { tests_passed++; textPrintAt(0, tests_passed + tests_failed, "PASS: " name); } \
    else      { tests_failed++; textPrintAt(0, tests_passed + tests_failed, "FAIL: " name); } \
} while (0)

u32 add32(u32 a, u32 b) { return a + b; }
u32 mul32(u32 a, u32 b) { return a * b; }
u32 shl32(u32 a, u8 n)  { return a << n; }
u32 shr32(u32 a, u8 n)  { return a >> n; }

void test_a7(void) {
    u32 r;
    fixed fr;
    r = add32(0xFFFFUL, 1UL);          TEST("add_carry",       r == 0x00010000UL);
    r = mul32(0x100UL, 0x100UL);       TEST("mul_high",        r == 0x00010000UL);
    r = shr32(0x12345678UL, 16);       TEST("shr_high_to_low", r == 0x00001234UL);
    r = shl32(0x18000UL, 1);           TEST("shl_low_to_high", r == 0x00030000UL);
    fr = fixDiv(FIX(1), FIX(5));       TEST("fixDiv_lib",      fr != 0);
}

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
    setScreenOn();
    textPrintAt(0, 0, "=== A7 REPRO ===");
    tests_passed = 0; tests_failed = 0;
    test_a7();
    textPrintAt(0, 26, "--------------------");
    textPrintAt(0, 27, tests_failed == 0 ? "ALL PASS - A7 SHIPPED" : "BUGS - A7 OPEN");
    while (1) WaitForVBlank();
    return 0;
}
```

Companion `/tmp/a7_repro/Makefile`:

```make
OPENSNES := /home/kobenairb/workspace/opensnes
TARGET   := a7_repro.sfc
ROM_NAME := A7 REPRO
CSRC     := main.c
USE_LIB  := 1
LIB_MODULES := console math text sprite dma
include $(OPENSNES)/make/common.mk
```

## Phase 1 — entry plan (NOT yet attempted)

### Scope — minimal acceptance set

Per STRUCTURAL_DEFECTS.md A7 §"Proposed fix" subset (1)–(3):

1. **Slot allocator widening** — `compiler/qbe/w65816/abi.c` (and possibly
   slot logic in `emit.c`): a `Kl` temp must reserve 4 bytes, not 2.
   Today `paroff = (parn + 1) * 2` at `abi.c:187` assumes ALL params are
   2 bytes; same flaw at the body-temp slot allocator.
2. **`Oload` / `Ostore` for `K == Kl`** in `compiler/qbe/w65816/emit.c`:
   emit a pair of 16-bit ops covering both halves (lo at `(addr)`, hi at
   `(addr+2)`), symmetric for store.
3. **`Oadd` / `Osub` for `K == Kl`**: emit carry-aware pair:
   ```
   lda lo(a); clc; adc lo(b); sta lo(result)
   lda hi(a);      adc hi(b); sta hi(result)
   ```
   (`clc` on first add only; second `adc` propagates carry).

Phase 1 acceptance gate: `add32(0xFFFFUL, 1UL)` returns `0x00010000UL`.
Cases 2-5 still BUG (Phase 2 = shifts/bitwise; Phase 3 = mul/div runtime).

### Cascade risk to monitor

The 30 existing QBE patches all interact with the slot allocator and
emit pass:
- A-cache through pha (chantier C.6, must stay valid for Kw)
- Dead store elimination (must not eliminate Kl high-half stores)
- Frameless-leaf opt (must NOT promote a Kl-using leaf to frameless
  if the high half lives in a temp slot)
- TCO (chantiers C.1/C.2.1/C.2.2, all interact with frame management)
- Cycle gate E2 hard-fails on >5% TOTAL or >25%+50cyc per-fn regression

Mandatory pre-merge validation after Phase 1:
1. `make clean && make` — full rebuild (cycle gate runs on examples)
2. `cd tools/opensnes-emu && node test/run-all-tests.mjs` — full
   non-quick suite (compiler-tests phase included)
3. `node test/run-benchmark.mjs` — 34-fn bench, no regression
4. Manual: rebuild `/tmp/a7_repro/`, observe 1/5 PASS (add_carry now OK)

### Risk-reduction strategy (recommended)

Rather than land all of Phase 1 at once, ship in 3 atomic patches:

- **1.a** Slot allocator widening only — params are correctly 4 bytes
  for Kl, body-temp slots are 4 bytes for Kl. Test fixture: compile a
  Kl-using function, verify slot offsets are sane. No bench regression
  expected (Kw path unchanged).
- **1.b** `Oload`/`Ostore` pairs for Kl — round-trip storage now correct.
  Test fixture: `u32 x = arg; return x;` should preserve full 32 bits.
- **1.c** `Oadd`/`Osub` carry-aware — `add_carry` case finally PASSES.

Each patch is independently bench-verifiable.

### Test infrastructure to wire IN PHASE 1 (not Phase 0)

When Phase 1 ships and add_carry case turns green, ALSO ship:
- New dir `tools/opensnes-emu/test/fixtures/runtime/u32_arithmetic/`
  with `test_u32.c` (the source from above, adapted to the unit-test
  template that uses `tests_passed`/`tests_failed` counters read by
  `phase5_runtime` via `core._bridge_read_wram`) plus its `Makefile`.
- The runner (`run-all-tests.mjs::phase5_runtime`) walks `unit/` and
  picks it up automatically — no harness changes needed.
- Initially the test is partially-green (1/5) until Phase 2/3 land. CI
  will report `unit/u32_arithmetic` as failure with `4 failed, 1
  passed`. Cleanest: ship Phase 1.c + the fixture in the same commit so
  CI is partial-RED but explicitly documented in the commit message
  ("4/5 cases still red — closes in A7 Phase 2/3").

## Why we stopped at Phase 0

- Session was already long (post-release + SDK review + benchmark
  re-baseline + develop sync). Compiler patch landing in queue of a
  long session is the canonical recipe for a 30-patch regression cascade.
- Phase 1 is ~2-3 days of focused work, not 1-2 hours.
- Phase 0's value (RED proof + handoff doc) is preserved fully here.

## Cross-references

- `.claude/STRUCTURAL_DEFECTS.md` lines 832-1050 (A7 entry, full proposed fix)
- `lib/source/math.c:92-100` (the `fixDiv` kernel, broken)
- `compiler/qbe/w65816/abi.c:187` (param offset bug)
- `compiler/qbe/w65816/emit.c:1409` per catalogue ref (Oadd, single 16-bit)
- `tools/opensnes-emu/test/phases/compiler-tests.mjs:1390-1424` (insufficient
  smoke check that should have caught this)
- BENCHMARK re-baseline commit `ccaf9c7` (2026-05-10) — A7 won't move
  the 34-fn bench since none of those use u32 arithmetic.
