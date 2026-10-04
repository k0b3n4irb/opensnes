---
name: A6+A7 leaf_opt slot overlap with 4-byte pointer params — root cause for 7 of 8 unit/collision failures
description: QBE w65816 slot allocator places Kl local at SP+14, overlapping the high half of a 4-byte pointer param. Affects every leaf function that takes a pointer arg and stores it locally. Found via mesen2-rpc + IR/ASM correlation in ~20 minutes.
type: project
---

# A6+A7 leaf_opt slot overlap bug

**Date**: 2026-05-14
**Found via**: autonomous compiler-level diagnostic, no Mesen2 GUI
**Time to root cause from "8 named test failures"**: ~20 minutes

## The bug in one sentence

In leaf_opt mode (framesize=0), the QBE w65816 slot allocator starts
assigning local slots at offset SP+(slot+1)·2 starting from slot=6 in
the rectInit case — directly overlapping the high half of the 4-byte
`r` pointer param at SP+14..15.

## Trace evidence

### IR (clean — what cproc emits)

```
function $rectInit(l %.1, w %.3, w %.5, w %.7, w %.9) {
@start.96
    %.2 =l alloc4 4
    storel %.1, %.2     # store r ptr into local Kl slot
    ...
@body.97
    %.11 =w loadw %.4   # load x param value
    %.12 =l loadl %.2   # reload r ptr from local
    %.13 =l extuw %.12  # extract low w (the offset half)
    %.14 =l extuw 0
    %.15 =l add %.13, %.14
    storew %.11, %.15   # *(r) = x
```

This is correct IR. The bug is downstream.

### Generated ASM (the smoking gun)

```asm
rectInit:
    rep #$20
@start.96:
    lda 12,s       ; load r param's LOW half from SP+12 ✓
    sta 14,s       ; STORE to SP+14 — but SP+14 IS r's HIGH half slot!
    lda 14,s       ; read SP+14 back — gets the LOW we just wrote
    sta 16,s       ; finalize the corrupted "high"
@body.97:
    lda 14,s       ; load low (corrupted but happens to be correct low)
    sta 26,s
    lda.w #0       ; zero-extend high half
    sta 28,s
    lda 10,s       ; load x value
    pha
    lda 28,s       ; ⚠️ X = HIGH half = 0
    tax
    pla
    sta.l $0000,x  ; writes to address $00:0000 — clobbers tcc__r0
```

Every subsequent field store (.y, .width, .height) follows the same
pattern: X is loaded from the HIGH half slot, which contains 0 because
of the bogus extuw. All four writes target $00:0000.

### Slot annotation confirms

```
; Function: rectInit (framesize=0, slots=20, alloc_slots=6, fn_leaf=1, leaf_opt=1)
; temp 64: slot=6, alloc=-1
```

temp 64 (the Kl %.2 holding the saved r ptr) has slot=6, which maps to
SP+(6+1)·2 = **SP+14**. The 4-byte r param occupies **SP+12..15**.
SP+14..15 = r's high half. Direct overlap.

## Why this is a post-A6 regression

Pre-A6, pointer params were 2 bytes. r param occupied SP+12..13 only.
SP+14 was the first byte BEYOND the params — safe for leaf_opt locals.

Post-A6 (commit f2aad9c, "A6.1 pointer 8/8 → 4/2 in type.c"), pointer
params are 4 bytes. r now occupies SP+12..15. SP+14 is no longer safe.

The QBE w65816 slot allocator's slot-numbering scheme was not updated
when pointer arg sizes changed. It still computes the first local slot
as if all params are 2 bytes wide.

## Why exactly 7 of 8 failures cluster here

Every failing test (except the corrupted "partial_ovZR00p" string)
calls a lib function that takes a `Rect *` or similar pointer:

| Test            | Lib fn called          | Leaf? | Has Kl local? |
|---|---|---|---|
| rectInit_x/y/w/h | `rectInit(Rect*, ...)` | yes   | yes (%.2)     |
| rectSetPos       | `rectSetPos(Rect*, ...)`| yes   | yes           |
| point_inside     | `collidePoint(x, y, Rect*)` | yes | yes        |
| point_edge       | `collidePoint(x, y, Rect*)` | yes | yes        |

Every one of these is a true leaf (no further calls), so `leaf_opt=1`
fires for every one of them. The common shape "leaf function with a
pointer arg that gets stored locally" is the perfect trigger.

## The fix sketch

Two viable paths:

### Option A — slot allocator: skip past actual params

In `compiler/qbe/w65816/emit.c` around line 3220 (slot assignment), when
`leaf_opt=1` and `framesize=0`, the first allocatable slot must skip
past the param area. Compute `param_bytes` (using A6-aware Kl=4 for
pointers, 2 for others), and set the starting `slot` so that
`(slot+1)·2 >= param_bytes + 4 (JSL return) + 1 (PHP)`. The function
already counts `count_fn_param_bytes(fn)` for other purposes — reuse
it as the floor for the slot index in this mode.

### Option B — disable leaf_opt for functions with Kl locals from pointer params

A more conservative fix: in the leaf_opt eligibility check
(emit.c:3262), additionally require that the function has no Kl
local that's an alloc-derived storage of a pointer param. Forces
those functions into the standard frame allocation path where slots
sit above the alloc area, never overlapping params.

Option B is smaller-scope and easier to validate. Option A is more
efficient (preserves leaf_opt for the affected functions, just shifts
their slots up by 2 bytes per pointer param).

Recommend Option B first to clear the residuals quickly, then revisit
A as a separate perf optimisation chantier.

## Reproduction without Mesen2 (~10 seconds)

```sh
$ grep -B2 -A4 "^rectInit:" \
    /home/kobenairb/workspace/opensnes/lib/build/lorom/collision.c.asm
rectInit:
    rep #$20
@start.96:
    lda 12,s     # r low
    sta 14,s     # ⚠️ same offset
    lda 14,s     # ⚠️ same offset
    sta 16,s
```

Any function whose generated ASM does `lda N,s; sta (N+2),s; lda (N+2),s`
within its first 4 instructions has this bug — that's the visible
signature.

## Verification plan post-fix

1. Apply the chosen fix to QBE.
2. Rebuild compiler: `make -C compiler/qbe && cp compiler/qbe/qbe bin/`.
3. Rebuild lib: `make clean && make lib`.
4. Rebuild the test fixture (CRITICAL — see [[a6_a7_unit_test_diagnostic]]
   for the stale-build trap I fell into earlier today):
   `cd tools/opensnes-emu/test/fixtures/unit/collision && \
    OPENSNES=/home/kobenairb/workspace/opensnes make clean && \
    OPENSNES=/home/kobenairb/workspace/opensnes SKIP_LINT=1 make`
5. Run the diagnostic: `node /tmp/diag_collision_fail_list.mjs` — should
   show 0 FAIL lines.
6. Quick suite: `cd tools/opensnes-emu && node test/run-all-tests.mjs --quick`
   → expect **269/269**.

## What's left

- The corrupted "partial_ovZR00p" string. This likely traces to the
  same family of bugs — leaf_opt collision with a different lib call.
  Re-check after fixing the rectInit/collidePoint issue; may auto-clear.
- The 2 `unit/text` failures — same diagnostic process, different ROM.
  Probably the same root cause (text fixture also exercises leaf
  functions with pointer params).

## Fix-attempt session findings (2026-05-14, second pass)

Attempted three fix approaches with mesen2-rpc-driven verification.
Each failed cleanly, but exposed that the bug is **deeper and more
architectural** than initially scoped.

### Attempt 1 — Enable Kl aliased reads (revert)

Skipped the local copy at `Oload Kl` from neg slot, removed the
`cls != Kl` exclusion in `emitload_adj`/`emit_load_high`. Idea: read
Kl params directly from caller frame at neg_slot / neg_slot+2.

**Regression**: 8 → 11 failures. The alias propagation chain
(`Ocopy` at emit.c:2218 propagates regardless of class) breaks down
when the copied Kl temp is subsequently modified: reads via alias
return the ORIGINAL param value, not the modified one. Containment +
same_position tests had been PASSING by coincidence with the broken
rectInit (both got zeroed memory that happened to satisfy the
checks); making rectInit correct exposed collideRect's downstream
miscompare.

### Attempt 2 — Swap load order in Oload Kl (revert)

Read HIGH half *before* writing LOW half in the param-copy sequence,
so the param's high half is captured before the local store
clobbers SP+14.

**Result**: emitted ASM looks correct in isolation. rectInit's first
call (from test_rect_collision, with args 0,0,16,16) writes
correctly: probed `r.x=0, r.y=0, r.w=16, r.h=16` ✓.

But the test_rect_helpers call (args 10,20,30,40) fails. Probe
confirmed `rectInit(&r, 10, 20, 30, 40)` writes `r.x=10` correctly
AT FIRST. Then at `@do_body.155` (the first `TEST("rectInit_x", ...)`),
`mem[SP+18]` (where caller's `%.1 = &r` lives) reads `$002B` instead
of the expected `$1F27`.

### Root cause — deeper than the swap fix

The slot overlap is **bidirectional**:

| Frame | Layout |
|---|---|
| `test_rect_helpers` SP=$1F25 | slot 8 (%.1=&r) low at SP+18 = **$1F37** |
| `rectInit` SP=$1F16 | slot 14 high at SP+32 = **$1F36..$1F37** |

When rectInit emits `sta 32,s` (slot 14 high half store, used for
the running address temp during the per-field store loop), it
writes to physical RAM $1F36..$1F37 — which is **the low byte of
caller's `%.1` slot**. After rectInit returns, the caller's stored
&r pointer has its low byte clobbered.

The fundamental issue: leaf_opt sets `framesize=0` and uses the
caller's stack for locals. Slot offsets are computed independently
in each function — neither side knows the other's layout, so
nothing prevents overlap. The current heuristic (skip the alloc-
shadow stores) gets the small case right but doesn't generalize to
"the function has any Kl temp needing storage".

### Two viable architectural fixes (defer to dedicated chantier)

#### Option C — Disable leaf_opt for any function with Kl computed temps

Strict: if `can_be_frameless` returns true but the function has any
Kl temp that's not aliased to a param (i.e., result of extuw, add,
load from alloc), force `leaf_opt=0`. The function gets a real
frame; its locals live below SP_callee, never overlapping caller's
frame.

Smaller blast radius than C, more invasive than B. Probably the
right balance.

#### Option D — Teach can_be_frameless about Kl temps properly

The existing `can_be_frameless` check considers a function frameless
if alloc-shadowed temps are the only allocs. Extend it to also
require: every Kl temp used as an indirect-store address is
representable via alias (i.e., the address derives only from a
param). Functions where the address is COMPUTED (e.g. `r + 2`) get
denied.

This preserves leaf_opt for the trivially-frameless cases (`return
*p`) while denying it for the troublesome ones (`*r = ...; *(r+2) =
...; ...`).

### Why we're stopping here

Either C or D is ~30-60 minutes of code + 3-5 iterations of
test-suite verification + Mesen2 spot checks. The audit suggests
this is the right next chantier session entry point, not a quick
extension of the current one.

Diagnostic scripts to keep around:
- `/tmp/diag_rect3.mjs` — basic post-rectInit r-state probe
- `/tmp/diag_rect5.mjs` — find-target-call + step-through
- `/tmp/diag_rect7.mjs` — read mem[SP+18] at TEST comparison entry

The conclusion from this session: **the bug is two-level slot
allocation conflict that no single emit-time swap can fix**. The
fix must redesign leaf_opt slot allocation to avoid caller-frame
overlap — Option C or D above. Defer to dedicated session.

## Cross-references

- [[mesen2_rpc_mvp_validated]] — what makes this diagnostic feasible
- [[mesen2_rpc_phase1_breakthrough]] — RPC architecture
- [[a6_a7_unit_test_diagnostic]] — initial failure enumeration via screen-buffer dump
- [[a6_a7_unified_audit]] — chantier audit (predicted this class of bug at line "the leaf-opt slot allocator must account for class/storage mismatch")
- Commit `5625889` — A6 acache breakthrough fix (related family)
- Commit `3fe4555` — A6.13 BANK0 threshold tuning
- `compiler/qbe/w65816/emit.c:3220` — the slot allocator to patch
- `compiler/qbe/w65816/emit.c:3262` — the leaf_opt eligibility check
