---
name: A7 Phase 1 design — Kl codegen lowering, change-site map
description: Detailed design doc for chantier A7 Phase 1. Maps every code site that needs modification to fix 32-bit (Kl) class arithmetic in the QBE w65816 backend. Builds on a7_phase0_handoff.md (RED proof). Implementation NOT started.
type: project
---

This document is the **design layer** for A7 Phase 1. Phase 0
(`a7_phase0_handoff.md`) has the RED proof and the high-level 3-patch
decomposition. This file maps every change-site in the QBE w65816
backend with file:line references and the required transformation.

**Read first:** `a7_phase0_handoff.md` (RED proof + scope) + the A7 entry
in `.claude/STRUCTURAL_DEFECTS.md:832-1050` (catalogue context).

## Design choice (locked)

**Lower Kl to pairs of Kw operations at emit time, not at IR time.**

QBE IR keeps Kl as a single typed op (`Oadd Kl`, `Oload Kl`, etc.). The
w65816 backend's emit pass interprets the class and lowers to a pair of
16-bit operations. This matches QBE's general design (each op carries
a class, backend interprets) and avoids touching IR-level passes that
the 30 existing patches depend on.

Alternative considered: lower at the abi/IR level (emit two Kw temps
per Kl temp). Rejected — would touch every existing optimization
(A-cache, dead store, leaf opt, TCO) because they all reason about
single Kw temps.

## QBE data model recap (relevant subset)

`compiler/qbe/all.h:204-213`:

```c
enum { Kx = -1, Kw, Kl, Ks, Kd };  /* Kw=0, Kl=1 */
#define KWIDE(k) ((k)&1)            /* Kl/Kd are wide */
```

`struct Tmp` (`all.h:345-372`) carries `slot` and `cls` per temp.
`struct Ins` (`all.h:230-248`) carries `op:30`, `cls:2`, plus
the chantier-A2 `volat` byte.

## Bug surface map (Phase 0 confirmed)

Each site is a place where the backend currently treats Kl as a single
16-bit value. Fix coverage required for Phase 1 (slot + load/store +
add/sub):

| # | Site | File:line | Current behaviour | Fix in patch |
|---|---|---|---|---|
| A | `maybeassign` slot allocator | `emit.c:988-996` | Increments `fn->slot` by 1 per temp regardless of class | **1.a** |
| B | `coalescephi` phi-result slot | `emit.c:1009-1021` | Same — single slot per temp | **1.a** |
| C | `framesize` calc | `emit.c:2859` | `(fn->slot + 1) * 2` — slot count in 16-bit units | **1.a** (no change, formula stays correct after slot widening; just verify) |
| D | `emitload_adj` primitive | `emit.c:1077-1151` | Single `lda <offset>,s` per call | **1.b** (add helpers `emitload_hi/lo`) |
| E | `emitstore` primitive | `emit.c:1163-1226` | Single `sta <offset>,s` per call | **1.b** (add helpers `emitstore_hi/lo`) |
| F | `Oload` handler (Oload + Oloadsw + Oloaduw) | `emit.c:2280-2328` | Single 16-bit load, falls through for all three ops | **1.b** (switch on `i->cls`; Kl path emits low+high pair) |
| G | `Ostorel` handler | `emit.c:2095-2141` | Stores low, **hardcodes high = 0** (see comment "near pointers") | **1.b** (read high from `r0` high half, not zero) |
| H | `Ostorew` handler | `emit.c:2143-2200` | Single 16-bit store | **1.b** (Ostorel path, Ostorew unchanged) |
| I | abi.c parameter loading | `abi.c:178-190` | `paroff = (parn+1)*2` — assumes Kw, emits `Oloadsw Kw` always | **1.a + 1.b** (byte-accumulator + pass `i->cls` to emit) |
| J | `Oadd` handler | `emit.c:1409-1438` | Single 16-bit `clc; adc` | **1.c** (switch on cls; Kl: pair `clc;adc;sta;lda;adc;sta`) |
| K | `Osub` handler | `emit.c:1440-1456` | Single 16-bit `sec; sbc` | **1.c** (switch on cls; Kl: pair) |
| L | Phi moves | `emit.c:2670, 2691` | Single 16-bit copy | **1.c** (defer to Phase 2 if no Kl phi in repro; verify) |

## Patch 1.a — Slot allocator widening

**Goal**: Kl temps reserve 4 bytes (2 slot indices) instead of 2 bytes
(1 slot). After this patch, slot offsets are correct but no codegen
change yet — high halves stay uninitialised.

### Change A: `maybeassign` (`emit.c:984-996`)

```c
static int
maybeassign(Ref r, Fn *fn, int slot)
{
    if (rtype(r) == RTmp && r.val >= Tmp0) {
        if (fn->tmp[r.val].slot < 0) {
            fn->tmp[r.val].slot = slot;
            slot++;
            /* Kl reserves 2 slots (4 bytes total). The high half lives
             * at slot+1 and is read/written by emit's Kl-aware pair
             * sequences (Oload Kl, Oadd Kl, etc.). */
            if (fn->tmp[r.val].cls == Kl)
                slot++;
        }
    }
    return slot;
}
```

### Change B: `coalescephi` (`emit.c:1009-1021`)

Same logic — when assigning a slot to a phi result, increment by 2 if
class is Kl. Use `p->cls` (phi has its own class field).

### Change C: `framesize` (`emit.c:2859`)

No code change — formula `(fn->slot + 1) * 2` already converts slot
indices to byte count, and our slot widening makes `fn->slot` count
each Kl temp as 2 slots. Verify the comment is still accurate; update
the comment to mention Kl reserves 2 slots.

### Change I (partial): abi.c param offset (`abi.c:178-190`)

Refactor to a byte accumulator that knows class:

```c
int paroff_bytes = 2;  /* offset 2 = first arg at SLOT(-2) = framesize+4,s */
for (i = &b->ins[b->nins]; i > b->ins;) {
    i--;
    switch (i->op) {
    case Opar:
    case Oparsb:
    case Oparub:
    case Oparsh:
    case Oparuh:
        /* Emit Oload with the param's actual class so emit pass
         * knows whether to lower to a pair sequence. */
        emit(Oload, i->cls, i->to, SLOT(-paroff_bytes), R);
        paroff_bytes += (i->cls == Kl) ? 4 : 2;
        break;
```

For Phase 1.a alone: keep the abi change minimal — just the byte
accumulator. The `emit(Oload, i->cls, ...)` switch happens here too
because abi.c can't emit two ops cleanly for one Opar (that would
require allocating a high-half temp, which is Phase 1.b territory).

**Acceptance gate 1.a**:
- `make compiler && make lib && make examples` clean
- Cycle bench unchanged (no Kl temps in 34-fn bench)
- `/tmp/a7_repro/` still 5/5 BUG (no codegen change yet)
- New diagnostic: build with `CC_DEBUG=A` (debug['A']) and verify Kl
  temps log slot count = 2 in the per-function header comment

## Patch 1.b — Oload / Ostore pairs for Kl

**Goal**: Storage round-trip works for Kl. Loading and storing a u32
preserves both halves.

### Change D: `emitload_lo / emitload_hi` helpers

Refactor `emitload_adj` to be the LOW-half primitive. Add a sibling
that emits the HIGH-half load (offset + 2 from the slot's base).

```c
static void emitload_lo(Ref r, Fn *fn) { emitload_adj(r, fn, 0); }
static void emitload_hi(Ref r, Fn *fn) { emitload_adj(r, fn, 2); }
```

The existing `emitload_adj(r, fn, sp_adjust)` already accepts a byte
offset. The +2 offset is naturally the high half. Verify slot-based
addresses also work with +2 (current line 1102:
`(slot + 1) * 2 + sp_adjust`).

### Change E: `emitstore_lo / emitstore_hi`

Same pattern. Note: `emitstore` doesn't have an `sp_adjust` parameter
today — add one OR add a sibling helper. Keep the existing emitstore
as `emitstore_lo`-equivalent (for Kw stores), add `emitstore_hi`.

### Change F: `Oload` handler

Currently lines 2280-2328 handle Oloadsw, Oloaduw, Oload all in one
case. Split:

```c
case Oloadsw:
case Oloaduw:
    /* Kw load — single 16-bit, unchanged */
    ...
case Oload:
    if (i->cls == Kl) {
        /* Kl load — pair: low half from address, high half from address+2 */
        emitload_kl_pair(r0, i->to, fn);
    } else {
        /* Kw Oload — same as Oloadsw/Oloaduw */
        ...
    }
    break;
```

`emitload_kl_pair`: load low from `r0` (slot/global/indirect),
emitstore_lo to dest; load high from `r0 + 2`, emitstore_hi to dest.

For RTmp source: source temp's slot+1 holds high half (via 1.a
widening). For RCon source (constant): split low/high from the
constant value.

### Change G: `Ostorel`

Lines 2095-2141. Today: emits low from `r0`, then **hardcodes high = 0**.
Replace the hardcoded zero with a load of `r0`'s high half:

```c
case Ostorel:
    /* Store Kl: pair sequence, both halves from r0 */
    emitload_lo(r0, fn);
    emit_kl_store_lo(r1, fn);  /* dest low */
    emitload_hi(r0, fn);
    emit_kl_store_hi(r1, fn);  /* dest high */
    acache_invalidate();
    break;
```

`emit_kl_store_lo/hi` is the helper that handles the various r1
shapes (alloc slot, vreg, RCon CAddr, RCon CBits, indirect) similarly
to the existing block at lines 2099-2138, but parameterised on which
half (offset 0 or +2 from base).

### Change H: `Ostorew`

No change — Kw stores stay single-op.

### Change I (completion): abi.c

After 1.a's byte accumulator, the `emit(Oload, i->cls, ...)` already
passes the class through. The Oload Kl handler from change F handles
the pair load when the source is `RSlot` (negative slot from caller's
frame). Verify the negative-slot path in `emitload_adj` handles +2
correctly:

```c
case RSlot:
    if (slot < 0) {
        /* Negative slot = parameter from caller's frame */
        fprintf(outf, "\tlda %d,s\n",
                framesize + PARAM_OFFSET + (-slot) + sp_adjust);
```

Yes — the formula naturally accommodates a +2 sp_adjust to get the
high half. No additional change needed.

**Acceptance gate 1.b**:
- `u32 x = arg; return x;` preserves all 32 bits
- `/tmp/a7_repro/` still 5/5 BUG (Oadd not yet fixed) — but storage
  round-trip is now correct, observable in the generated `.asm`
  (look for `lda 6,s; sta ...; lda 8,s; sta ...+2` patterns instead
  of the previous single load)
- Cycle bench: ZERO regression on the 34 Kw-only functions; the new
  Kl path is unreachable from the existing bench

## Patch 1.c — Oadd / Osub carry-aware for Kl

**Goal**: `add_carry` case in `/tmp/a7_repro/` flips BUG → PASS.

### Change J: `Oadd`

After loading r0_low + adc r1_low, A holds the low half + carry. The
carry must NOT be cleared before the high-half adc. The 65816 `adc`
instruction reads C and writes C, so a chained `adc` works directly:

```c
case Oadd:
    if (i->cls == Kl) {
        emitload_lo(r0, fn);    /* lda r0_low */
        fprintf(outf, "\tclc\n");
        emit_adc_lo(r1, fn);    /* adc r1_low */
        emitstore_lo(i->to, fn); /* sta result_low */
        emitload_hi(r0, fn);    /* lda r0_high */
        emit_adc_hi(r1, fn);    /* adc r1_high — carry preserved */
        emitstore_hi(i->to, fn); /* sta result_high */
        acache_invalidate();    /* A-cache state is complex, just invalidate */
        break;
    }
    /* ...existing Kw path (INC/DEC opt, A-cache, etc.)... */
```

Important: the `acache_invalidate()` is necessary because the existing
Kw optimisations (INC/DEC, A-cache through pha) all assume a single
load+adc+sta sequence. Mixing those with Kl pairs would create cache
corruption — invalidate to be safe. Cycle cost: marginal (the few
instructions that read from A-cache after a Kl op now do a fresh load).

### Change K: `Osub`

Same pattern with `sec; sbc; sbc`. Note: 65816 `sbc` uses C as
**borrow-inverted** (1 = no borrow, 0 = borrow). After first `sbc`
sets/clears C correctly, second `sbc` reads it and propagates.

```c
case Osub:
    if (i->cls == Kl) {
        emitload_lo(r0, fn);
        fprintf(outf, "\tsec\n");
        emit_sbc_lo(r1, fn);
        emitstore_lo(i->to, fn);
        emitload_hi(r0, fn);
        emit_sbc_hi(r1, fn);
        emitstore_hi(i->to, fn);
        acache_invalidate();
        break;
    }
    /* ...existing Kw path... */
```

### A-cache disable

For all Kl operations (1.b + 1.c), invalidate the A-cache before AND
after. The existing A-cache logic doesn't model "A holds the high
half of temp X" — adding it would be a chantier of its own. Safe
default: pessimistic invalidation around Kl.

**Acceptance gate 1.c**:
- `add_carry` test in `/tmp/a7_repro/` passes (`r == 0x00010000UL`)
- `mul_high`, `shr_high_to_low`, `shl_low_to_high`, `fixDiv_lib` all
  still BUG (Phase 2 = shifts/bitwise, Phase 3 = mul/div runtime)
- Full test suite (`node test/run-all-tests.mjs --quick`) green
- Cycle bench unchanged (the Kw paths are untouched)

## Test infrastructure (ship with 1.c)

When 1.c lands, also add the runtime fixture:

```
tools/opensnes-emu/test/fixtures/unit/u32_arithmetic/
├── Makefile           (LIB_MODULES := console math text sprite dma)
└── test_u32.c         (the source from a7_phase0_handoff.md, adapted
                        to use static u8 tests_passed/tests_failed
                        symbols read by phase5_runtime via
                        core._bridge_read_wram)
```

The runner walks `unit/` automatically. Initially the test reports:

```
unit/u32_arithmetic: 1 passed, 4 failed
```

This is intentional — Phase 2 and 3 close the remaining 4 cases.
Document this in the commit message:

```
Cycle-Regression-OK: A7 Phase 1.c expected 4/5 unit/u32_arithmetic
failures; closes in A7 Phase 2 (shifts) + Phase 3 (mul/div runtime).
```

The `Cycle-Regression-OK` trailer is the documented override path for
chantier E2 — even though the bench cycle gate isn't tripping (Kw
paths unchanged), the unit-test partial-RED is similar in spirit and
deserves the explicit override.

## Cascade-risk specifics (must verify per patch)

The 30 QBE patches that interact with these sites:

| Patch | Interaction | Mitigation |
|---|---|---|
| **A-cache through pha** (`ea06b2f`, chantier C.6) | A-cache models single-Kw value in A; Kl ops break that | Invalidate around Kl ops (already designed in) |
| **Dead store elimination** (`82fcf9d`, Phase 7a) | Skips stores when slot never read | For Kl, must NOT skip if either low OR high read; safest: don't dead-store-eliminate Kl at all in 1.c |
| **Frameless leaf opt** (`8f49c2f`, Phase 3) | Promotes leaves with no real slots to frameless | If a leaf has Kl temps that need slot+1 high half, the frameless promotion is wrong; gate on "no Kl temps" in `is_frameless_safe()` |
| **TCO** (`ed840fb`, `e98ce21`, `8a0c971`, `744bcdf`) | Tail call uses caller's arg slots; for Kl args, must overwrite both halves | Phase 1 doesn't add new TCO sites; existing TCO already handles 4-byte arg if the lowering emits two `pea`s — verify |
| **Comp + branch fusion** (`ab31dd5`) | 16-bit comparison; Kl comparisons not in scope until Phase 2 | No interaction in 1.a/b/c |
| **Lazy rep #$20** (`bab0164`) | Mode tracking; Kl pair adds nothing new | No interaction |

**Recommended verification order per patch**:
1. `make clean && make compiler` — qbe builds clean
2. `make lib examples` — lib + examples build, no link errors
3. `node test/run-all-tests.mjs --quick` — 269/269 pass
4. `node test/run-benchmark.mjs` — bench TOTAL within ±5 cyc of pre-patch
5. Build `/tmp/a7_repro/` — observe expected acceptance gate
6. If all green, commit + push

If a regression appears, **bisect within the patch** — the 1.a/1.b/1.c
split is designed for granular bisection.

## Cross-references

- A7 catalogue entry: `.claude/STRUCTURAL_DEFECTS.md:832-1050`
- Phase 0 handoff: `.claude/notes/chantiers/a7_phase0_handoff.md`
- Repro source: `/tmp/a7_repro/main.c` (recreatable from handoff)
- BENCHMARK doc: `docs/BENCHMARK.md` (post-`ccaf9c7` re-baseline)
- Cycle gate policy: `docs/BENCHMARK.md:131-180` (E2 hard gate, override
  trailer mechanism)

## What this design does NOT cover

- **Phase 2 — Shifts / bitwise**: `Oshl`, `Oshr`, `Osar`, `Oneg`, `Oxor`,
  `Oand`, `Oor` for Kl. Each needs its own pair-emission logic with
  cross-half propagation (shifts especially are tricky — shift-by-N
  for N >= 16 swaps halves, etc.).
- **Phase 3 — Mul/div runtime**: `__mul32`, `__sdiv32`, `__udiv32` ASM
  helpers in `lib/source/runtime.asm`, plus `Omul/Odiv/Omod` lowering
  to `jsl __mul32` etc. for Kl class.
- **Comparisons** (`Oceqi`, etc.) for Kl — not in scope until Phase 2,
  but worth flagging.
- **Function pointer indirection bank byte** — that's chantier A6,
  separate from A7.
