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
