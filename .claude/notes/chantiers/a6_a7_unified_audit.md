---
name: A6+A7 unified chantier — audit phase findings
description: Comprehensive audit of every site touching pointer ABI (8B IR) and u32 arithmetic (Kw class with ILOADL/ISTOREL mismatch) in cproc + QBE w65816. Maps the structural fix that closes both A6 and A7. Audit date 2026-05-10.
type: project
---

This is the **audit-phase deliverable** for the unified A6+A7 chantier
decided on 2026-05-10. After the failed Phase 1.a attempt earlier the
same day (slot widening for all Kl temps caused frame overflow due to
pointer-Kl coexistence), we committed to the structural fix instead of
the workaround.

**Status**: audit complete. Implementation has NOT started. Next-session
reopen reads this doc + acts on the recommendations below.

## Critical insight surfaced during audit

The catalogue's A7 entry says u32 arithmetic is broken because "QBE
w65816 32-bit (Kl class) codegen never extended past 16-bit". This is
**not quite right**.

In current cproc IR (`compiler/cproc/qbe.c:186-219`), u32 (4-byte int)
maps to:

```c
case 4: return ... (struct qbetype){'w', 'w', ILOADL, ISTOREL};
```

That is: **class 'w' (Kw)** but **data type 'w' (DW = 4 bytes)** with
load/store ops `ILOADL`/`ISTOREL`. The class ≠ the storage size.

Pointers (`compiler/cproc/qbe.c:208-209`) map to:

```c
if (t->kind == TYPEPOINTER) return l;  /* class 'l' = Kl */
```

— with size 8 today (per `mkpointertype` / `type.c:81-82`).

So the QBE IR has **two different "wide types"**:

| C type | Storage size | QBE class | QBE data | Backend lowers as |
|---|---|---|---|---|
| `u32` / `long` | 4 bytes | Kw (16-bit class) | DW (4 bytes) | **single 16-bit ops** ← bug |
| `pointer` (today) | 8 bytes | Kl (long class) | DL (8 bytes) | single 16-bit ops + 6 bytes wasted padding |
| `pointer` (after A6) | 4 bytes | Kl | DL (will be 4) | needs pair lowering for proper bank-byte access |
| `u64` / `long long` | 8 bytes | Kl | DL (8 bytes) | currently broken, out of A7 Phase 1 scope |

The bug for u32 is that the **w65816 backend treats class Kw as
strictly 16-bit** (slot=2 bytes, single 16-bit ops). cproc emits the
storage as 4 bytes (DW data type), but the SSA-level operations are
class Kw. Result: storage is 4 bytes, arithmetic processes only the
low 16.

**Implication for the chantier**: A7 cannot be fixed in isolation. The
clean fix is to change cproc so u32 uses class **Kl** (with DW data,
size 4) — uniform with post-A6 pointers (Kl, DL=4 data, size 4). Then
the w65816 backend's Kl pair lowering handles both uniformly.

## A6+A7 unified — structural change set

### A6 — Pointer ABI 8B → 4B + indirect-call bank byte

#### Site A6.1 — `mkpointertype` (`compiler/cproc/type.c:74-87`)

Today:
```c
struct type *
mkpointertype(struct type *base, enum typequal qual)
{
    struct type *t;
    t = mktype(TYPEPOINTER, PROPSCALAR);
    t->base = base;
    t->qual = qual;
    t->size = 8;     /* ← change to 4 */
    t->align = 8;    /* ← change to 2 */
    if (base)
        t->prop |= base->prop & PROPVM;
    return t;
}
```

Fix: `t->size = 4; t->align = 2;` (24-bit address + 1 byte alignment;
align = 2 for 16-bit-word alignment on the target).

#### Site A6.2 — `typenullptr` (`compiler/cproc/type.c:48`)

```c
struct type typenullptr = {.kind = TYPENULLPTR, .size = 8, .align = 8, .prop = PROPSCALAR};
```

Fix: `.size = 4, .align = 2`.

#### Site A6.3 — Stale "treats 'l' as 16-bit" comment (`compiler/cproc/qbe.c:106-108, 207`)

Today the comment reads:

```c
/* The w65816 backend treats 'l' as 16-bit since that's the pointer size. */
```

This was written when the backend stored only the low 2 bytes of an
8-byte pointer in the slot. After A6 it's misleading — the backend
will store 4 bytes (24-bit + alignment). Update comment.

#### Site A6.4 — `dtype_size[DL]` (`compiler/qbe/emit.c:47-52`)

```c
static int dtype_size[] = {
    [DB] = 1,
    [DH] = 2,
    [DW] = 4,
    [DL] = 8     /* ← change to 4 */
};
```

After A6, DL represents 4-byte pointers and 4-byte longs (which today
already use DW class, but DL is also used for non-scalar passing).
Setting DL=4 collapses pointer + long into the same data type.

**Caveat**: `long long` (8 bytes per C99) still uses class Kl with
data DL. If we set DL=4, `long long` storage becomes wrong.

**Resolution**: keep DL=8 for `long long`, introduce DL=4 specifically
for pointers. Either:
- (i) Add a new data type `DP` (pointer, 4 bytes) so `long long` keeps
  DL=8 and pointers use DP=4
- (ii) Change cproc's pointer mapping to use class 'w' with data 'w'
  (DW=4) — same as u32 — and revisit the C.5 padding accordingly
- (iii) Use class Kl uniformly for both pointers AND `long long`, but
  vary the BYTE size based on the type (requires per-temp size info,
  not just class)

Option (ii) is the cleanest: pointers become Kw class with DW data,
matching post-fix u32. The Kl class is reserved for true 8-byte
`long long`. This means **A6 is really "switch pointers from Kl/DL
to Kw/DW"**, not just "shrink Kl from 8 to 4".

#### Site A6.5 — C.5 padding patch (`compiler/qbe/emit.c:170-240`)

After A6 with option (ii) above, pointers emit `.dl <symbol>` (3 bytes)
into a DW slot (4 bytes). Padding is 1 byte. The C.5 lookup tables
`reftype_emit_size` and `numtype_emit_size` need to be revised:

| Type | reftype (.dl symbol) | numtype (.dl number) | dtype_size | pad |
|---|---|---|---|---|
| DB | 1 | 1 | 1 | 0 |
| DH | 2 | 2 | 2 | 0 |
| DW | 3 (sym = 24-bit) | 4 (lit = 32-bit) | 4 | 1 / 0 |
| DL | (n/a after A6, only `long long`) | 8 | 8 | 0 |

So after A6: most padding goes away, only `.dl <symbol>` for a pointer
field needs 1 byte of `.dsb 1, 0`.

#### Site A6.6 — Indirect-call bank byte (`compiler/qbe/w65816/emit.c:2564-2580`)

Today (broken):

```c
} else {
    /* Indirect call: function pointer in temp/slot.
     * Load the 16-bit address, store to DP scratch (tcc__r9),
     * set bank byte to current bank ($00), then jml [tcc__r9].
     */
    emitload_adj(r0, fn, argbytes);
    fprintf(outf, "\tsta.b tcc__r9\n");
    emit_sep20();
    fprintf(outf, "\tlda #$00\n");           /* ← HARDCODED zero bank */
    fprintf(outf, "\tsta.b tcc__r9+2\n");
    emit_rep20();
    fprintf(outf, "\tphk\n");
    fprintf(outf, "\tpea ++-1\n");
    fprintf(outf, "\tjml [tcc__r9]\n");
    fprintf(outf, "++\n");
}
```

After A6 (correct): read the bank byte from the pointer storage's
+2 offset (the third byte of the 4-byte pointer):

```c
} else {
    emitload_adj(r0, fn, argbytes);          /* low 16 of fn ptr */
    fprintf(outf, "\tsta.b tcc__r9\n");
    emitload_adj(r0, fn, argbytes + 2);      /* bank byte at +2 */
    emit_sep20();                            /* switch to 8-bit A */
    fprintf(outf, "\tsta.b tcc__r9+2\n");
    emit_rep20();
    fprintf(outf, "\tphk\n");
    fprintf(outf, "\tpea ++-1\n");
    fprintf(outf, "\tjml [tcc__r9]\n");
    fprintf(outf, "++\n");
}
```

The `emitload_adj(r0, fn, sp_adjust + 2)` reads the high half of the
pointer. Since pointers post-A6 are 4 bytes (low 16 at slot, high 16
at slot+2 with bank byte in low 8 of high 16), this gives us the bank
byte in A's low 8 after the SEP #$20. Correct.

**Caveat**: this requires the pointer's high half to actually contain
the bank byte. cproc must initialise pointer storage with the full
24-bit address. That part is already done by C.5 (which fixed the
padding); after A6 the same path emits `.dl <sym>` for 3 bytes in a
4-byte slot.

#### Site A6.7 — Pointer arithmetic (`compiler/cproc/qbe.c:985-990`)

```c
if (e->type->kind == TYPEPOINTER) {
    if (e->u.binary.l->type->kind != TYPEPOINTER)
        ...
    if (e->u.binary.r->type->kind != TYPEPOINTER)
        ...
}
```

Pointer arithmetic (`p + offset`, `p - q`) extends the integer operand
to pointer size. With pointer size 8→4, this changes the extension op
emitted (e.g. from `extuw` to nothing if the integer is already 4
bytes). Audit each branch.

### A7 — u32 arithmetic + class realignment

#### Site A7.1 — Switch u32 to class Kl

`compiler/cproc/qbe.c:213-216`:

```c
case 2: return wh;
case 4: return t->prop & PROPFLOAT ? s : (struct qbetype){'w', 'w', ILOADL, ISTOREL};
case 8: return t->prop & PROPFLOAT ? d : l;
```

Fix for u32 (and pointer post-A6): use class 'l' uniformly for 4-byte
non-float types:

```c
case 4: return t->prop & PROPFLOAT ? s : (struct qbetype){'l', 'w', ILOADL, ISTOREL};
```

This makes 4-byte int (u32, long) emit class Kl in QBE IR. The
w65816 backend's Kl-pair lowering (next sites) handles them properly.

But wait — this conflicts with `long long` (8 bytes) which also uses
'l'. The backend can't distinguish them by class alone. Two options:

- (i) Track the actual storage size in `Tmp.width` field. The backend
  reads `width` in addition to class. width=Wfull(4) vs width=Wfull(8)
  distinguishes u32 from u64.
- (ii) Push the class divergence: u32 uses class Kw (16-bit, current)
  and u64 uses Kl (32-bit logical). The backend lowers Kw with width-4
  storage to "pair Kw" sequences. u64 stays unimplemented in Phase 1.

Option (i) requires the backend to interpret `width` consistently —
non-trivial change to all emit sites that read class.

Option (ii) is more localised: change the u32 case to keep class 'w'
but the backend learns to recognise "Kw with DW data type" as the
4-byte path. Simpler but uglier.

**Proposed**: option (i). Add a w65816-specific helper
`tmp_byte_size(Fn *fn, int tmp)` that returns 2 (Kw narrow), 4 (Kw
wide / Kl narrow), or 8 (Kl wide), based on class + width or tracked
explicitly via cproc IR metadata. All emit sites consult this helper.

#### Site A7.2 — Slot allocator widening (`compiler/qbe/w65816/emit.c:988-996, 1009-1021`)

After A6 unifies pointer + u32 to 4 bytes, slot widening can be
trivially uniform: any Kl temp (or any Kw temp with width=4 if we
go option (ii)) reserves 2 slots.

This is the same code change attempted in the failed Phase 1.a, but
now SAFE because pointer-Kl temps are also 4 bytes (same size, same
slot count).

#### Site A7.3 — `Oload` Kl pair (`compiler/qbe/w65816/emit.c:2280-2328`)

Switch on class + width. For Kl 4-byte (or Kw 4-byte): emit pair load.

#### Site A7.4 — `Ostorel` corrected (`compiler/qbe/w65816/emit.c:2095-2141`)

Replace hardcoded `lda.w #0` for high half with read from r0's high
half.

#### Site A7.5 — `Oadd` / `Osub` carry-aware (`compiler/qbe/w65816/emit.c:1409-1456`)

Switch on class. Kl pair sequence: `clc; adc lo; sta lo; adc hi; sta hi`.

#### Sites A7.6 — `Oshl/Oshr/Osar/Oneg/Oxor/Oand/Oor` Kl pair sequences

Each needs a pair lowering. Shifts especially: shift-by-N where
N >= 16 is a swap (`hi := lo, lo := 0` for Oshl) plus shift the swapped
half by N-16. Shift-by-N where N < 16 needs cross-half carry/borrow.

#### Sites A7.7 — Comparisons Kl

`Oceq, Ocne, Ocsgt, Ocsge, Ocsle, Ocslt, Ocugt, ...` for Kl: two-half
compare (high first, branch on equality, then low). Sign vs unsigned
matters for the high-half compare.

#### Sites A7.8 — `__mul32`, `__sdiv32`, `__udiv32` runtime helpers

`lib/source/runtime.asm` gains three new helpers. Sketches:

- **`__mul32`**: 16×16→32 hardware multiplier (`$4202/$4203 → $4216/$4217`)
  applied 4 times to compute the four partial products of (a_lo × b_lo,
  a_lo × b_hi, a_hi × b_lo, a_hi × b_hi). Combine into low 32 bits of
  the product. Estimated ~80-120 cycles. Wire to `Omul Kl` lowering in
  emit.c.
- **`__udiv32`**: shift-and-subtract long division on 32-bit dividend
  by 32-bit divisor, returning 32-bit quotient. Estimated ~200-300
  cycles per call. Wire to `Oudiv Kl`.
- **`__sdiv32`**: signed wrapper around `__udiv32`. Negate operands
  if needed, return signed result. ~30 cycles overhead.

These ASM helpers are added in lib/source/runtime.asm and exposed via
WLA-DX symbols; QBE w65816 emit pass lowers `Omul/Odiv/Omod Kl` to
`jsl __mul32` / etc.

## A1 investigation reproduction (the 9 failures)

The catalogue says A1 tested pointer size = 2 (way too small for 24-bit
addresses) and got 9 test failures. We should reproduce these to:

1. Confirm the failure modes match the analysis above
2. Identify any failures NOT predicted by the analysis (there may be
   surprises in C.5-related code paths or function pointers in
   non-zero banks)

The catalogue cites a `1 baseline drift on basics/random` for the
pointer=8 baseline (current). After A6 (pointer=4 with proper
indirect-call), this drift should resolve.

**Action for the implementation phase**: do a one-shot test build with
pointer.size=4 / align=2 BEFORE shipping any patches. List every
failure. Plan a fix per failure. Target: 0 failures after A6 ships.

## Cross-references

- A6 catalogue entry: `.claude/STRUCTURAL_DEFECTS.md:611-830`
- A7 catalogue entry: `.claude/STRUCTURAL_DEFECTS.md:832-1050`
- Phase 0 handoff: `.claude/notes/chantiers/a7_phase0_handoff.md`
- Phase 1.a redesign log: `.claude/notes/chantiers/a7_phase1_design.md`
- C.5 padding patch: qbe `5fe27f0`
- A1 (already shipped): `.claude/STRUCTURAL_DEFECTS.md:184-277`

## Implementation order (next sessions)

A clean multi-session sequence:

1. **Session A6-1** (3-5 days focus): A6.1 + A6.2 + A6.3 + A6.4 +
   A6.5 + A6.6 + A6.7. Reproduce the 9-fail set early to validate.
   Acceptance: full test suite green at pointer=4. Cycle bench within
   ±5%.
2. **Session A7-1** (2-3 days): A7.1 (u32 → Kl class) + A7.2 (slot
   widening). Acceptance: u32 storage round-trip works; bench
   unchanged.
3. **Session A7-2** (2-3 days): A7.3 + A7.4 + A7.5. Acceptance:
   `add_carry` test in `/tmp/a7_repro/` PASSES; sub works; storage
   round-trip works.
4. **Session A7-3** (3-4 days): A7.6 (shifts/bitwise) + A7.7
   (comparisons). Acceptance: all 4 synthetic cases pass.
5. **Session A7-4** (3-4 days): A7.8 (runtime helpers) + lib audit
   (`fixDiv`, `fixLerp`, etc.). Acceptance: `fixDiv_lib` test passes;
   5/5 unit/u32_arithmetic green.
6. **Session A7-5** (1-2 days): docs + CHANGELOG + cycle bench
   re-baseline + ship.

Total: ~3 weeks effective time across ~5 sessions. Calendar 4-6 weeks
realistic.

## Why we audit before implementing

The 2026-05-10 Phase 1.a failure (slot widening cascade on pointer-Kl
coexistence) was caused by **insufficient audit**. The original design
doc listed 6 cascade-risk patches but missed the pointer-Kl class
sharing — which IS the cascade-risk-of-record on this chantier.

This audit fixes that gap by mapping every site that touches pointer
ABI or u32 ABI explicitly. Implementation can now proceed knowing the
edges of the change set.

## Open questions for next session

1. **DW vs DL data type for pointers post-A6**: option (ii) at A6.4
   (pointers use Kw/DW like u32, Kl reserved for u64) needs a
   prototype to confirm it doesn't break QBE's handling of `long long`
   in non-65816 targets (we don't ship those, but the patch lives in
   our QBE fork).
2. **`tmp_byte_size` helper signature** at A7.1: should it be a single
   field on Tmp (added by cproc), or computed by walking class+width
   in the backend? Decision affects how invasive the patch is.
3. **9-fail reproduction**: reproduce immediately when A6 implementation
   starts — failure modes will dictate fix order within A6.

---

## 2026-05-10 A6-1 implementation attempt — findings (rolled back clean)

The first A6-1 implementation session (2026-05-10) tried the change set
above with the structure proposed in the audit. Net result: **rolled
back, baseline restored, 269/269 tests pass**. The session uncovered
**two coupled blockers** that need their own sub-chantier before
A6-1 can ship cleanly.

### What was attempted

Changes applied (in this order, building on each other):

1. **A6.1 + A6.2** — `mkpointertype()` and `typenullptr` size 8→4,
   align 8→2 in `compiler/cproc/type.c`.
2. **A6.4** — `dtype_size[DL]` 8→4 in `compiler/qbe/emit.c`. Result:
   ALL DL items shrink to 4 bytes, including `long long` (broken if
   any code uses it; today no example does).
3. **A6.6** — indirect-call bank byte read from pointer storage
   `+2` instead of hardcoded `lda #$00` in
   `compiler/qbe/w65816/emit.c`.
4. **Slot widening** — `maybeassign()` and `coalescephi()` reserve 2
   slots for Kl temps (correct codegen for runtime pointer temps).
5. **Pointer→DW** — refined cproc `qbetype()` so pointers emit data
   type 'w' (DW=4 bytes) instead of 'l' (DL=8). Reverted A6.4 in
   favour of this since DL stays 8 for `long long`.

### Blocker A — 9 failures on A6 alone (root cause not yet diagnosed)

With A6.1 + A6.2 + A6.4 applied (no slot widening, no pointer→DW
yet), the quick test suite reports **9 failures** in the same shape
the A1 catalogue investigation predicted:

| Phase | Examples failing | Severity |
|---|---|---|
| Visual regression | `basics/aim_target` (774 px), `basics/random` (355 px), `basics/timer` (521 px), `graphics/backgrounds/mode1` (57344 px), `graphics/backgrounds/mode1_bg3_priority` (57344 px) | mixed; mode1 = whole screen wrong |
| Lag detection | `basics/random`, `basics/scene_stack`, `basics/timer`, `input/controller` — all 100% lag frames | severe (examples hung) |

Adding A6.6 (indirect-call fix) did NOT change the failure count.
Reverting `dtype_size[DL]=4` and switching pointers to DW data type
ALSO did not change. Same 9 failures persist.

**Root cause hypothesis**: pointer Kl temps in runtime stack slots
have only 2 bytes of slot space (1 slot index × 2 bytes/slot), but
the IR claims they're 4 bytes. The high half (bank byte) is
**uninitialised** in the slot. Pre-A6 the indirect-call hardcoded
bank=0, masking the issue for bank-zero functions. Post-A6 the
indirect-call reads a garbage bank byte.

But that doesn't explain `mode1` (57344 pixels different = whole
screen wrong, no indirect call involved). There's likely a SECOND
mechanism — possibly cproc emitting different IR shapes for some
pointer arithmetic patterns. Needs deeper investigation in the next
session.

### Blocker B — Slot widening triggers 8-bit stack offset overflow

Adding slot widening (correct codegen for pointer temps):

```
goombainit framesize: 168 → 260 bytes
                      → wlalink rejects `lda 272,s`
                      → INPUT_NUMBER: Out of 8-bit range
```

`examples/games/mapandobjects/goomba.c::goombainit` has ~46 Kl temps
post-widening, totaling 129 slots × 2 = 258 bytes + alignment = 260.
The 65816's `lda <off>,s` addressing mode caps offsets at 8 bits
(0–255), so wlalink rejects the assembly outright.

This is **not specific to goombainit** — any future C code with
> ~30 simultaneous pointer locals hits the same wall.

### NEW sub-chantier — A6.8 (large-frame addressing)

Without solving Blocker B, A6 cannot ship even after the 9-failure
investigation completes. The 65816 hardware has no native > 8-bit
stack-relative addressing; we need a **fallback addressing mode** for
oversized frames.

**Approach**: when `framesize > 256`, the function prologue copies SP
into a DP scratch register (e.g., `tcc__r10`). All stack-relative
accesses become `lda (tcc__r10),y` with the 8-bit offset moved into
Y (or use `(tcc__r10),y` indirect with Y = offset). Slow but correct.

**Cost**: ~3 extra cycles per stack access for affected functions.
Acceptable since these are by definition complex / pointer-heavy
functions.

**Effort estimate**: 5-7 days. Touches `framesize` calc, prologue
emission, AND every site that emits `lda/sta/cmp/adc <N>,s`. Has to
keep the 8-bit-offset fast path for normal-sized frames.

**Status**: not yet started. A6-1 BLOCKED on A6.8.

### Updated implementation order

| Step | Scope | Dependency |
|---|---|---|
| **A6-pre-1** | Investigate the 9 failures: which mechanism causes the visual diffs on `mode1` etc. (independent of slot widening / indirect-call) | none |
| **A6.8** | Large-frame addressing mode (TSC + DP indirect for frames > 256) | none — independent backend chantier |
| **A6-1** | Apply A6.1 + A6.2 + indirect-call fix + slot widening + pointer→DW | A6.8 (so widening doesn't overflow) AND A6-pre-1 (root-cause the 9 failures) |
| A7-1 | u32 → Kl class + slot widening already done by A6-1 | A6-1 |
| A7-2…A7-5 | (unchanged from earlier in this doc) | A7-1 |

A6-1 is no longer 3-5 days. With A6-pre-1 + A6.8 + A6-1 itself,
**A6-pre-1+A6.8+A6-1 sequence is ~2 weeks** (A6.8 alone is 5-7 days).

### Files touched and reverted (2026-05-10)

```
compiler/cproc/type.c        — mkpointertype + typenullptr size/align (REVERTED)
compiler/cproc/qbe.c         — qbetype pointer→DW (REVERTED)
compiler/qbe/emit.c          — dtype_size[DL] (REVERTED)
compiler/qbe/w65816/emit.c   — indirect-call bank byte (REVERTED)
compiler/qbe/w65816/emit.c   — slot widening for Kl (REVERTED)
```

All five reverts confirmed clean: `git status` shows no diff on the
submodules; `make examples` builds; `node test/run-all-tests.mjs
--quick` reports 269/269. Submodule SHAs match `compiler/PINS.md`
(verify-toolchain green).

### A6-pre-1 root-cause diagnosis (2026-05-10, second session)

The 9 failures predicted by the A1 catalogue investigation have a
**single root cause** — not a multi-mechanism puzzle as originally
suspected. The `mode1` whole-screen failure that seemed to defy the
"bank byte" hypothesis is actually the same mechanism as the lag
failures, just with a different visible symptom.

**Root cause (NEW SITE)**: `compiler/qbe/w65816/emit.c:2452-2515`
(`Oarg` handler) **always pushes 2 bytes** regardless of the
argument's class:

```c
case Oarg:
case Oargsb: ...:
    /* ... single pea.w or single pha ... */
    argbytes += 2;  /* All args pushed as 16-bit */
```

This is correct for Kw args (16-bit values) but **wrong for Kl args**
(32-bit values, including post-A6 pointers). With pointer ABI 4 bytes
(post-A6.1), the caller pushes only 2 bytes for a pointer arg, but
the callee reads 4 bytes from the stack — the upper 2 bytes contain
whatever the next pushed arg landed on, or stale frame data.

Evidence captured during investigation: `mode1`'s `main()` emits
`bgLoad(0, &bg, 0, 0x4000, 0x0000)` as:

```asm
pea.w 0          ; arg 0: bg (u8) — correct
pea.w bg         ; arg 1: &bg (POINTER, should push 4 bytes, only pushed 2)
pea.w 0          ; arg 2
pea.w 16384      ; arg 3
pea.w 0          ; arg 4
jsl bgLoad
adc.w #10        ; cleanup: 5 args × 2 bytes = 10  ← assumes ALL Kw
```

If the pointer arg were correctly pushed as 4 bytes, the cleanup
should be `adc.w #12` (5 Kw args × 2 + 1 ptr arg × 4 — but actually
all the OTHER args ARE Kw so cleanup of Kw-only would be 4 args × 2
+ 1 Kl × 4 = 12 bytes). Today's cleanup of 10 confirms the all-Kw
assumption.

Inside `bgLoad`, `asset->tiles_end` and other field accesses use the
asset pointer's high half (bank byte) for `lda` indirect operations.
With high half corrupted, every dereference of `asset->field` reads
random memory. The DMAs to VRAM emit garbage data → mode1 whole
screen wrong (57344 px diff).

Same mechanism explains:
- `lag/scene_stack` (calls indirect through Scene struct fn ptrs —
  scene struct lookup goes through asset ptr arg)
- `lag/timer`, `lag/random`, `lag/controller` (call lib functions
  passing pointer args — corrupted on the way in)
- `visual/aim_target` (smaller diff — only some sprites/UI affected)
- `visual/random`, `visual/timer` (rendering relies on ptr-arg
  passing, partially survives on luck)

**This is the SINGLE bug behind all 9 failures.** Fix specification:

#### NEW Site A6.9 — Class-aware Oarg push

`compiler/qbe/w65816/emit.c:2452-2515` (Oarg + Oargsb/ub/sh/uh
fall-through) needs to push 2 bytes for Kw and 4 bytes for Kl:

```c
case Oarg:
    if (i->cls == Kl) {
        /* push high half first, then low half (so callee reads
         * low half at lowest offset, matching little-endian C order
         * where ptr->byte[0] is the LSB) */
        emitload_adj(r0, fn, argbytes + 2);  /* high half */
        fprintf(outf, "\tpha\n");
        emitload_adj(r0, fn, argbytes);      /* low half */
        fprintf(outf, "\tpha\n");
        argbytes += 4;
    } else {
        /* existing Kw single-push path */
        ...
        argbytes += 2;
    }
    break;
```

For RCon CAddr (the `pea.w bg` case): need WLA-DX syntax for the
bank byte of a symbol. Likely:

```asm
pea.w :bank(bg)        ; push bank in low 8 of high 16
pea.w bg               ; push low 16 of address
```

WLA-DX `:bank(sym)` operator is the canonical way (verify in
`.claude/WLA-DX.md` before implementing). Alternative: add a runtime
"address-of" helper that constructs the 4-byte pointer value.

#### Cascade: cleanup math + indirect-call

`compiler/qbe/w65816/emit.c:2581-2611` Ocall cleanup uses `argbytes`
already — it just needs the corrected accumulator. No code change
beyond Oarg.

`compiler/qbe/w65816/emit.c:2570-2575` indirect-call (A6.6 site):
loads function pointer from caller's stack via `emitload_adj(r0, fn,
argbytes)`. Today reads 2 bytes; needs 4 for proper bank byte
(already documented in A6.6).

### Updated A6 fix sequence

A6 (post-pre-1 diagnosis) requires THREE coordinated changes that
all must ship together:

1. **A6.1** — pointer 8→4 bytes in cproc (already designed)
2. **A6.4** — `dtype_size[DL]` 8→4 in QBE (already designed)
3. **A6.6** — indirect-call bank byte from pointer storage (already designed)
4. **A6.9 [NEW]** — Oarg pushes 4 bytes for Kl args (this session)
5. **Slot widening** — Kl temps reserve 2 slots (to receive the 4-byte arg correctly)
6. **A6.8** — large-frame addressing mode (so slot widening doesn't overflow goombainit)

A6.9 is the missing piece. With it, the slot widening attempt would
succeed because pointer temps get 4 bytes pushed by callers AND
4 bytes of slot to receive them in.

A6.8 (large frames) is still independent and still required — even
post-A6.9, slot widening for goombainit's 30+ pointer temps inflates
the frame past 256 bytes.

**Revised effort**: A6.9 is small (~half day, mostly WLA-DX bank
operator research). A6.8 is the dominant cost (5-7 days).

### A6.1+A6.4+A6.6+A6.9 attempt (2026-05-11, rolled back) — A6.10 + A7.3 discovered

After A6.8 shipped (commit `4aa4989`, large-frame addressing + Kl slot
widening, 269/269 green), the next session attempted to layer the
remaining A6 sub-chantiers in one commit:

- A6.1: cproc `mkpointertype` size/align 8/8 → 4/2
- A6.4: qbe `dtype_size[DL]` 8 → 4
- A6.6: indirect-call emit reads bank byte from pointer storage
- A6.9: Oarg pushes 4 bytes for Kl args (with WLA-DX `:sym` operator
  for bank byte of RCon CAddr symbol args)

Result: **49 failures** (up from the 9 pre-fix baseline). Rolled back
cleanly to A6.8-only state, 269/269 restored.

### Root cause: A6.10 + A7.3 are prerequisites

The 49 failures trace to **two new bugs uncovered by the partial fix**:

1. **A6.10 — `compiler/qbe/w65816/abi.c` parameter loading**.
   Today emits `emit(Oloadsw, Kw, i->to, SLOT(-paroff), R)` for every
   `Opar*` regardless of the parameter's actual class. For a Kl param
   (post-A6 pointer), the caller now correctly pushes 4 bytes, but the
   callee's abi.c only loads the low 16 into the Kl temp's slot. The
   high half (bank byte) stays uninitialised in the (newly-widened)
   slot — every dereference of the param-loaded pointer reads garbage
   bank byte.

   Plus: `paroff = (parn + 1) * 2` assumes 2 bytes per param. For a Kl
   param, subsequent param offsets are off by 2 bytes (read shifted
   data).

   Fix: byte accumulator + emit `Oload` (not `Oloadsw`) with `i->cls`.
   But `Oload Kl` lowering itself is broken — see A7.3.

2. **A7.3 — `Oload` handler in emit pass does not pair-load Kl.**
   `compiler/qbe/w65816/emit.c:2280-2328` falls through `Oload`,
   `Oloadsw`, `Oloaduw` to a single `emitload + emitstore` path. Both
   helpers are class-agnostic (always 16-bit). A `Oload Kl` thus
   produces a single 16-bit load into the Kl temp's slot, leaving
   the high half uninitialised — same bank-byte loss as A6.10.

   Fix: switch on `i->cls`, emit pair load (low at addr, high at
   addr+2) and pair store (to i->to slot offset 0 and +2).

Without BOTH A6.10 and A7.3, post-A6 pointer ABI is non-functional
end-to-end — even if A6.1/A6.4/A6.6/A6.9 are perfectly correct in
isolation, the missing A6.10 + A7.3 break the param-loading reception
path. Caller and callee disagree on Kl param size.

### Cascade: A7.5 (Oadd Kl) also surfaces

For pointer arithmetic like `&asset->gfx` or `asset->tilemap` (where
`tilemap` is a Kl field at non-zero offset), cproc emits `Oadd Kl`
(pointer + scaled offset, both extended to Kl class). The current
`Oadd` handler at `emit.c:1409-1438` is also class-agnostic — emits
a single 16-bit add. For Kl, the high half of the result is
unchanged (no carry from low add propagates to high half).

This means lib functions like `bgLoad` that compute field addresses
via pointer arithmetic on the param-loaded asset pointer get a
double bug: garbage high half from A7.3 + lost carry from A7.5.

### Revised A6 + A7 minimum atomic set

The four sub-chantiers (A6.1+A6.4+A6.6+A6.9) need three more to ship
together as one functional unit:

- **A6.10** abi.c parameter loading: byte accumulator + class-aware
  emit (~30 min)
- **A7.3** Oload handler: pair load for Kl class (~1 hour)
- **A7.4** Ostorel handler: read r0's high half instead of hardcoded
  zero (~30 min)
- **A7.5** Oadd / Osub handlers: pair add/sub with carry for Kl class
  (~1 hour)

Total atomic patch: ~3-4 hours focused work. Each sub-site is small;
the coupling is what makes them an atomic unit.

The cascade is now well-bounded. After this set ships:
- The 9 catalogue failures should close (root cause was pointer arg
  passing + reception — A6.9 + A6.10 + A7.3 + A6.6 cover the whole
  path)
- A7.6+ (shifts, bitwise, mul/div) remain for u32 arithmetic but
  pointer ABI itself is closed
- B5 fixed32 unblocks

### Lessons captured

The audit doc's original framing assumed A6 would need ~3-5 days
of focused implementation. The reality after one session of
implementation is:

- The 9 failures predicted by the A1 investigation reproduce reliably
  but their root cause is NOT a single mechanism — `mode1` whole-
  screen failure cannot be explained by indirect-call bank byte.
- The slot widening (which the original Phase 1.a discovered as
  needed) requires solving the 8-bit stack offset overflow first. This
  is **a hardware constraint** of the 65816's addressing modes, not
  a compiler design flaw — but it must be addressed before the
  backend can faithfully store 4-byte pointers in stack slots.
- The audit-phase doc captured the 15 sites correctly but missed two
  systemic issues (the multi-mechanism 9-failure case and the 256-byte
  stack offset wall). Both surfaced only by attempting the
  implementation. **The audit was necessary but not sufficient.**

The right next move is **A6-pre-1 (diagnose the 9 failures) + A6.8
(large-frame addressing)** as parallel sub-chantiers, before
re-attempting A6-1.

## 2026-05-11 9-site atomic patch attempt — rolled back, scope expanded

### What was attempted

After A6.8 shipped (commit 4aa4989) and the prior session's audit
update (commit e2ba69c), this session attempted the full 8-site
atomic patch in a single integrated compiler edit:

- A6.1 cproc/type.c mkpointertype 8/8 → 4/2 + typenullptr
- A6.4 qbe/emit.c dtype_size[DL] 8 → 4
- A6.6 qbe/w65816/emit.c indirect-call bank byte from `:sym`
- A6.9 qbe/w65816/emit.c Oarg push 4B for Kl args (with WLA-DX `:sym`)
- A6.10 qbe/w65816/abi.c byte accumulator + class-aware Oload emit
- A7.3 qbe/w65816/emit.c Oload Kl pair (low + high at addr/addr+2)
- A7.4 qbe/w65816/emit.c Ostorel reads r0's actual high half
- A7.5 qbe/w65816/emit.c Oadd/Osub Kl with carry propagation

Plus three helper functions `emit_load_high`, `emit_store_high`,
`emitop2_high` for Kl-pair semantics across slot/RSlot/RCon paths.

### Two prerequisites surfaced during testing

1. **A6.11 — `Oext*` for Kl destination (NEW)**.
   `Oextsb/Oextub/Oextsh/Oextuh/Oextsw/Oextuw` with Kl destination
   class need to initialise the high half:
   - Zero-extending variants (Ouw/Oub/Ouh): high = 0
   - Sign-extending variants (Osw/Osb/Osh): high = -1 if low<0 else 0

   Without this, every C `array[i]` pattern (where `i` is u16/s16 and
   `array` is a Kl pointer) constructs a pointer with garbage high
   half. Detected by inspecting `examples/text/hello_world` asm —
   the loop `tile = message[i]` emitted `lda i_low,s; lda.w #message;
   adc.w #:message` reading uninitialised slot bytes for the bank
   contribution.

2. **Dead-store optimisation + leaf-opt aliasing incompatibility
   with Kl temps**. Three optimisation paths in `emitstore`
   (`temp_is_dead_store`, `temp_alias`, `skip_dead_retstore_temp`)
   skip the slot write when "A still has the value for the next
   consumer". This assumption holds for Kw single-store/single-read
   patterns but BREAKS for Kl pair handling — between the low-half
   store and the next instruction's first load, the high-half write
   necessarily clobbers A. The Kw-optimised path leaves the LOW slot
   uninitialised; the Oadd Kl that follows reads garbage.

   Fix: add `fn->tmp[r.val].cls != Kl` guard to all three skip paths
   in `emitstore` and to the `temp_alias` skip path in
   `emitload_adj`. Trades a few cycles of optimisation for correctness
   on Kl arithmetic.

### Test result: 221/267, 46 failures (down from 267→269 baseline)

Despite shipping all 9 sites plus the 4 Kl optimisation guards, the
test suite regressed:

- `boot/basics/scene_stack` reported "DMA channel 7 clobbered" —
  something in the running game writes to $4370+. Specific cause
  not identified.
- 13 visual regressions in `audio/snesmod_*`, `basics/*`, `games/*`
  with pixel diffs ranging from 494 to 57344 (= full-screen).
- 7 lag-frame failures (game stuck in init/loop) including
  `text/hello_world` (100% lag frames).

The hello_world generated asm for `message[i]` reads correctly after
A6.11 + Kl-guards. The lib's `dmaCopyVram` heuristic
(`lda 10,s; cmp #$80; bcc...`) survives the 4-byte push order because
byte 10,s still contains the high byte of the 16-bit low half — the
LoROM bank-0 case continues to work.

The remaining 46 failures could not be diagnosed without an emulator
debugger session. The pattern is too broad to bisect from build
artifacts alone.

### Scope analysis: chantier is larger than audit predicted

This is the **3rd cycle** of the audit-implement-discover-prerequisite
loop for A6+A7:

| Cycle | Audit predicted | Implementation surfaced |
|---|---|---|
| 1 (2026-05-10) | A6.1+A6.4+A6.6 atomic | A6.8 large-frame prereq |
| 2 (2026-05-10) | A6.1+A6.4+A6.6+A6.9 | A6.10 + A7.3 + A7.4 + A7.5 prereqs |
| 3 (2026-05-11, this session) | A6.1+4+6+9+10 + A7.3+4+5 | A6.11 + Kl-opt guards + ~46 unexplained failures |

The remaining unexplained failures most plausibly involve:
- Lib ASM functions that take pointer args may have edge cases beyond
  the LoROM bank-0 heuristic (e.g., `dmaCopyVramBank` reads `lda 9,s`
  for bank byte — under 4-byte push this is now the LOW BYTE of low
  half, not the bank).
- Other Kl IR ops not handled in this session: `Ocopy Kl`, `Ostore`
  variants other than Ostorel, comparisons of Kl, `Omul`/`Odiv` of
  pointers (unusual but possible).
- The DMA channel 7 clobber suggests an out-of-bounds write through
  a wrong-bank pointer landing on hardware registers — diagnostic
  requires stepping the failing example.

### Rolled back cleanly to e2ba69c + A6.8 baseline

User decision (2026-05-11): rollback rather than commit WIP. Reasoning:
the structural fix discipline is more valuable than partial progress
on develop. Future re-attempt requires a longer focused session with
emulator-debugger access for diagnosis of the residual failures.

### Files reverted this session

- `compiler/cproc/type.c` (A6.1)
- `compiler/qbe/emit.c` (A6.4)
- `compiler/qbe/w65816/emit.c` (A6.6, A6.9, A6.11, A7.3, A7.4, A7.5,
  helpers, Kl optimisation guards)
- `compiler/qbe/w65816/abi.c` (A6.10)

`bin/qbe` and `bin/cproc-qbe` rebuilt to A6.8 baseline. 269/269 green
restored. develop unchanged at e2ba69c after this rollback (this audit
doc update is a separate commit).

### Next session entry point

A future session should plan for **8-12 hours focused work** with
emulator-debugger access (Mesen2 step-through). The 8 audit sites +
A6.11 + 4 Kl-opt guards are well-understood; the residual 46
failures need empirical diagnosis. A working hypothesis to test
first: **disable A6.4 (keep dtype_size[DL] = 8)** while keeping
A6.1 (C-side ptr size = 4). This decouples ROM data layout from
the IR layout change and lets us isolate whether the failures come
from compiler emission or from data section width.

---

## 2026-05-14 Mesen2 diagnostic + breakthrough

Re-attacked the chantier after rebasing `wip/a6-a7-atomic-v3` onto
develop tip (qbe 5c23467, post-2-pass + macOS arm64 SIGBUS fix +
Windows MinGW fix). Replay scaffolding applied cleanly on the new
base; reproduced the 221/267 baseline.

Phase-1 hypothesis tests (each ran the full `--quick` suite with
`BANK0_FAIL_THRESHOLD=0` to bypass the 3 examples that consume
extra const space under A6):

| Test | Pass/Fail count | Delta |
|---|---|---|
| Full replay (no further mods) | 221/267 | baseline |
| Revert A6.4 alone (`[DL]=8`) | 222/267 | +1 |
| Revert w65816/emit.c hunks only | 226/267 | +5 |
| Revert w65816/abi.c hunks only | 210/266 | -11 (worse) |
| Targeted `dmaCopyVram` bank-byte lib ASM fix | 223/267 | +2 |

The abi.c revert getting worse confirms those hunks are correct
(Kl param loading with byte accumulator). The emit.c hunks contain
some imperfections but aren't the major culprit. The bug is
elsewhere.

### The breakthrough — Mesen2 step-through on text/hello_world

User ran Mesen2 (`Debug → Register Viewer`) on the failing
hello_world ROM. PPU state at runtime:

```
$2100.0-3 Brightness   = 0
$2100.7 Forced Blank   = true
```

→ The screen is force-blanked because `setScreenOn()` never ran.
Three break-pause captures showed PC bouncing inside
`main@while_body_28` (the message-write loop), with `LDA $08,S`
reading the loop counter `i = $AD17` after millions of cycles —
massively out of range for a 12-character message buffer.

Generated-asm trace of `tile = message[i]`:

```asm
lda 8,s            ; A = i_low
sta 14,s           ; slot 14 = i_low (Kl low half of extended i)
lda.w #0
sta 16,s           ; slot 16 = 0 (Kl high half of extended i)
                   ; A = 0 NOW
clc
adc.w #message     ; A = 0 + #message   ← BUG
sta 18,s           ; slot 18 = #message_low (no `i` added)
lda 16,s
adc.w #:message
sta 20,s           ; slot 20 = #message_bank
lda 18,s
tax
sep #$20
lda.l $0000,x      ; tile = message[0] every iteration
```

The pointer arithmetic computes `0 + message` instead of
`i + message`. `i` is never added — the loop reads `message[0]`
forever, which is not 0xFF, so the loop never terminates.

### Root cause: stale A-cache after emit_store_high

The 4 ext ops (Oextub, Oextsh, Oextuh, Oextsw) with Kl destination
all emit:

```c
emitload(r0, fn);          // A = src_low
emitstore(i->to, fn);      // sta to dst's low slot; acache_set(dst)
if (i->cls == Kl) {
    fprintf(outf, "\tlda.w #0\n"); // A = 0 (or high half)
    emit_store_high(i->to, fn);    // sta to dst's high slot
}
```

After this, A holds the HIGH-half value (e.g. 0) but the acache
still says "A = dst's full value". Any subsequent
`emitload(dst, fn)` hits `acache_has(dst) = true`, skips the load,
and operates on the stale A. The Kl Oadd that consumes the
extended `i` then computes `0 + pointer` instead of `i + pointer`.

### Fix (5 lines, 1 site)

`compiler/qbe/w65816/emit.c` — `emit_store_high` now invalidates the
acache on entry:

```c
static void
emit_store_high(Ref r, Fn *fn)
{
    int slot;
    if (req(r, R)) return;
    acache_invalidate();   /* ← the fix */
    ...
}
```

Result: **221/267 → 259/269** (−36 failures, 80 % of the gap closed
in one fix).

### Remaining 10 failures (chantier A6.12+ split)

```
basics/random — 105 px
games/mapandobjects — 57344 px (full screen)
graphics/backgrounds/mode7_perspective — 26530 px
graphics/effects/gradient_colors — 33656 px
graphics/effects/parallax_scrolling — 21119 px
graphics/effects/transparent_window — 17304 px
graphics/effects/window — 2329 px
maps/mapscroll — 57344 px (full screen)
maps/slopemario — 31882 px
maps/tiled — 57344 px (full screen)
```

Concentration in maps + HDMA effects. Suspected lib ASM gaps:
DMA wrappers other than dmaCopyVram still hardcode bank-0 logic,
HDMA setup expects 16-bit pointer push width, etc. Per the
README's abort condition, lib ASM rework splits into chantier
A6.12+ rather than bundling into this patch.

Three additional examples (likemario, mapandobjects, tetris) hit
the bank-`$00` 16-byte fail-threshold ratchet — A6 consumes more
const space than develop. Separate from the codegen bug.

### Current state

- `cproc wip/a6-a7-atomic-v3` @ `9e2a5d6` — A6.1 (ptr size 4/2)
- `qbe wip/a6-a7-atomic-v3` @ `2bab670` — 9-site patch + acache fix
- Main `wip/a6-a7-atomic-v3` — submodule pointers need bumping +
  PINS update. Not committed yet.

DO NOT merge to develop until:
1. Lib ASM A6.12 chantier closes the remaining 10 failures.
2. Bank-`$00` impact addressed for the 3 ratchet-hitting examples.
3. Audit doc reviewed by maintainer.

---

## 2026-05-14 (late): residual basics/random — triaged as baseline drift, not A6 bug

After A6.12 + A6.13 closed 9 of 10 residual failures, `basics/random`
remained at 105-px diff. Root cause investigation:

`lib/source/console.c` initializes the PRNG seed from PPU hardware
counters in `consoleInit()`:

```c
rand_seed = REG_OPHCT | (REG_OPVCT << 8);
if (rand_seed == 0) rand_seed = 0xACE1;
```

The seed therefore depends on the exact PPU H/V counter values at the
moment `consoleInit()` executes. Any change in compiled code size or
boot sequence cycle count (which A6 induces via 4-byte pointer pushes
at every `pea.w :sym ; pea.w sym` call site) shifts those counters
and produces a different deterministic PRNG sequence.

The 105-px diff is the rendered hex+decimal pair `current_value`
displayed at row 12/14 after 120 frames — different seed → different
sequence → different displayed numbers.

This is **expected baseline drift**, not a regression. Confirmed by
`tools/opensnes-emu/test/BASELINES.md`:

> `rom_sha256` is the **drift signal**: if the current ROM hashes
> differently, a noisy diff is expected — the example was rebuilt
> since this baseline was captured.

Our baseline `tools/opensnes-emu/test/baselines/basics_random.meta`
records `rom_sha256: 0cae05a3...` (pre-chantier). Current ROM hashes
differ post-A6.

### Action

NO code fix. Regenerate the baseline as part of the pre-merge
cleanup, alongside any other example whose ROM SHA shifted under
A6+A7. Per `BASELINES.md`'s procedure:

```sh
cd tools/opensnes-emu
node test/run-all-tests.mjs --capture-baselines basics_random
# Visual-spot-check the new baseline in Mesen2 reference
git add test/baselines/basics_random.{bin,meta}
git commit -m "test(visual): regenerate basics_random baseline (A6+A7 ROM hash drift)"
```

This must NOT happen on wip; baseline regen is part of the dev->main
merge sequence so the baseline matches the final shipped binary.
