---
name: A1 follow-up — `long` is class Kw in cproc IR, not Kl
description: Post-A1 chantier (`int=2 long=4` shipped 2026-05-08), cproc still maps 4-byte `long` to QBE class `'w'` (= Kw, 16-bit on SNES backend) instead of `'l'` (= Kl, 32-bit pair). All long arithmetic gets silently 16-bit-truncated. Five distinct symptoms surfaced in B5 (fix32) and mode7_racing (A6+A7 codegen). Proper fix requires class remap + Kl handler implementation for every arithmetic op currently Kw-only.
type: chantier
---

## Status

**Investigation complete (2026-05-15)**, fix deferred. Single-session
patch attempt to handle `Oshl Kl` proved insufficient: the underlying
issue is that cproc never emits Kl IR for `long`-typed values, so any
Kl-class handler in the QBE backend is dormant code. The real fix is
multi-session (Class A, weeks).

## Symptoms (5 reproduced)

1. **`(long)literal << 16` constant-folded to 0**

   ```c
   long a = (long)40 << 16;           /* expect 0x280000 */
   /* assignment to global: correct (sink+2 = 40, sink = 0) */
   take_long((long)40 << 16);         /* expect push 40, 0 */
   /* compiled as: pea.w 0 ; pea.w 0   ← BOTH HALVES ZERO */
   ```

2. **`long += long_const` folded to `s16_counter++`**

   ```c
   long angle = 0;
   long vel = (long)1 << 14;          /* expect 0x4000 */
   while (1) { angle += vel; ... }
   /* compiled as: lda 6,s ; inc a ; sta 16,s   ← single-byte increment */
   ```

3. **`(long)s16 << 8` loses bits crossing the 16-bit boundary**

   ```c
   extern const s16 sine_table[256];
   long v = (long)sine_table[64] << 8;  /* expect 256 << 8 = 0x10000 */
   /* compiled as: lda sine_table+128 ; xba ; and #$FF00 ; sta ...
    *              low = 0; high = stack garbage */
   ```

4. **`long × long` calls truncating `__mul16`**

   ```c
   long result = a * b;
   /* compiled as: jsl __mul16   ← 16-bit truncated result */
   ```

5. **`long * 65536L` folded to 0**

   Same root cause as #1; the literal-multiply path collapses too.

## Root cause

`compiler/cproc/qbe.c:215`:

```c
case 4: return t->prop & PROPFLOAT ? s : (struct qbetype){'w', 'w', ILOADL, ISTOREL};
```

For a 4-byte non-float type (= `long` post-A1), cproc maps to QBE
class `'w'`. The QBE w65816 backend interprets `'w'` (= Kw) as a
**16-bit single-slot value**. The size in cproc's type table is 4
bytes — but the arithmetic emit path treats it as 16-bit, so every
`long` op silently truncates.

For comparison: pointers (4 bytes post-A6 chantier) map to class
`'l'` (line 108: `static const int ptrclass = 'l';`). The SNES
backend's Kl path (the `i->cls == Kl` branches scattered through
`emit.c`) was designed for this 32-bit pair representation. Pointer
arithmetic works because of those handlers; `long` arithmetic doesn't
because the IR is Kw and the Kl handlers never fire.

## What the fix requires

### Part 1 — cproc IR remap (one-line change, huge blast radius)

```c
case 4: return t->prop & PROPFLOAT
        ? s
        : (struct qbetype){'l', 'l', ILOADL, ISTOREL};  /* was 'w', 'w' */
```

That single change makes every `long` operation emit Kl IR. The lib's
`math.h` typedefs (`s32`, `u32`) become Kl-class. Every example
that uses `s32` for return values, locals, arrays, struct fields,
function args — **all** flip to Kl.

### Part 2 — Kl handlers in QBE w65816 backend

Currently the Kl path is implemented for:

| Op | Status | Source |
|---|---|---|
| `Oload`, `Ostore` | ✅ | emit.c `Ostorel`, A6 retrofit |
| `Oadd`, `Osub` | ✅ | emit.c:1641-1696 (A7.5 carry/borrow) |
| `Oarg` (push) | ✅ | emit.c:2938-2946 (Kl high then low) |
| Indirect call | ✅ | A6.6/9/11 bank-byte preservation |
| Param load (`%t =l ...`) | ✅ | A6.10 byte-accumulator path |
| `Oshl` | ❌ | falls through to Kw path → truncates |
| `Oshr`, `Osar` | ❌ | same |
| `Omul` | ❌ | calls `__mul16` → truncates |
| `Odiv`, `Orem` | ❌ | unclear what happens; probably fault |
| `Oand`, `Oor`, `Oxor` | ❌ | Kw path only handles low 16 |
| `Oneg` | ❌ | Kw path negates only low half |
| `Ocmpl*` | ❌ | comparisons truncated |
| `Oextsw`, `Oextuw` (16→32) | ❌ | extension paths missing |

Each missing handler needs:
- Detect `i->cls == Kl`
- Emit low-half op + high-half op + cross-half carry (for arithmetic)
  or independent halves (for bitwise)

The `Omul Kl` case is the worst — full 32×32→32 software multiply
using the SNES 8×8 hardware multiplier ($4202/$4203) needs 16 partial
products + accumulation + sign correction. Estimated ~310 cycles per
call (see chantier-B5 abandoned `fix32Mul` asm impl for reference).

### Part 3 — Validation

Every example built. Class A change. Mesen2 manual on the full set.
Full quick suite + opensnes-emu-emu visual regression. The 410-test
suite will exercise every shipping path that touches `s32`.

Expected fallout — some optimisations probably regress because they
were tuned against the silently-truncating Kw path:

- Compiler-bench cycle measurements likely shift (some up, some
  down).
- Several `KNOWN_LIMITATIONS.md` entries may be obsolete or refined.
- The post-A6 indirect-call bank-byte preservation might need an
  audit if it interacted with the `'w'`/`'l'` mismatch.

## Why deferred

Effort estimate from STRUCTURAL_DEFECTS.md was conservative — the
audit was based on A1 = "fix int sizes only", and pointers (A6) were
peeled off as the more invasive sibling. But the post-A1 reality is
that `long` arithmetic is ALSO broken in a way A1 didn't notice, and
the breakage is structurally identical to A6's: missing Kl handlers
in the backend.

The 5 symptoms here are not bugs in cproc constant-folding (as
initially diagnosed during the B5 chantier session) — they're bugs
in how cproc maps `long` to QBE class, combined with backend missing
the corresponding Kl paths.

Single-session work cannot ship this cleanly. The asm patch I tried
(adding `Oshl Kl` with `cnt >= 16` handling) was logically correct
but never activated, because cproc never emitted Kl IR for the test
case. Reverted at session end.

## Pre-conditions for re-opening

- Free engineering window of ~1-2 weeks
- Test suite baseline frozen (no parallel chantiers active)
- Mesen2 access for manual validation of representative examples
- Bench baseline regenerated post-merge (cycle deltas expected)

## Useful artifacts from the investigation

- 5 minimum-reproduction `.c` files at `/tmp/a1f_bug1/test*.c`
  (regenerate if /tmp is wiped: see the bug-list section above for
  the source patterns).
- The `Oshl Kl` patch sketched in `compiler/qbe/w65816/emit.c`
  (reverted) — kept in the session transcript for re-use.
- The `fix32Mul` asm in `lib/source/math.asm` from the B5 attempt —
  reverted, but the 4-byte × 4-byte multiply pattern (15 hardware-mult
  partial products + sign correction) is the right shape for the
  eventual `Omul Kl` handler in QBE.

## Cross-references

- `STRUCTURAL_DEFECTS.md` § A1 — original 1-week estimate (revised
  here to weeks based on missing-Kl-handler scope)
- `STRUCTURAL_DEFECTS.md` § A6 — sister chantier for pointers; its
  Kl-handler implementations are the template
- `STRUCTURAL_DEFECTS.md` § B5 — the chantier that exposed these
  symptoms (deferred indefinitely until this is done)
- `compiler/ABI.md` — type sizes table; updates needed once Kl
  remap lands
- `KNOWN_LIMITATIONS.md` "🟢 int and long type sizes match" entry —
  the "🟢" status is misleading; `long` SIZE matches but `long` SEMANTICS
  silently truncate to 16-bit. Either refine the entry to call out
  the limitation, or wait for the proper fix to land.

## When this rule does NOT apply

- Code using `s16`/`u16` only (most of the SDK lib and most examples
  — that's why nothing visibly broke when A1 shipped).
- Code using `long long` (8 bytes, presumably mapped correctly to Kl
  or higher — not verified but the symptoms above don't apply).
- Pointer arithmetic (already on Kl path after A6 work).
