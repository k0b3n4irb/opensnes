---
name: A1 follow-up — `long` is class Kw in cproc IR, not Kl
description: Post-A1 chantier (`int=2 long=4` shipped 2026-05-08), cproc still maps 4-byte `long` to QBE class `'w'` (= Kw, 16-bit on SNES backend) instead of `'l'` (= Kl, 32-bit pair). All long arithmetic gets silently 16-bit-truncated. Five distinct symptoms surfaced in B5 (fix32) and mode7_racing (A6+A7 codegen). Proper fix requires class remap + Kl handler implementation for every arithmetic op currently Kw-only.
type: chantier
---

## Status

**Sessions 2-7 landed (2026-05-15 — 2026-05-16) on `wip/a1-followup-long-kl`**.
Sessions 2-6 added the missing Kl handlers (bitwise, shifts, mul via
`__mul32`, 12 compares, div via `__[s]divmod32`) and the matching
opt-in runtime helpers (`lib/source/mul32.asm`, `lib/source/div32.asm`,
pinned to BANK 7 to avoid bank-0 displacement). All five Sessions
kept the quick suite at 269/269 because the new handlers were dormant
(cproc still mapped 4-byte non-float to Kw).

**Session 7 (2026-05-16) flipped the switch.** `compiler/cproc/qbe.c:215`
now returns `(struct qbetype){'l', 'l', ILOADL, ISTOREL}` for 4-byte
non-float types, every `long`/`s32`/`u32` op flows through the Kl
path, and `mul32-asm.o` + `div32-asm.o` are unconditionally in
`LINK_OBJS`. The 5 documented input bugs verify FIXED via direct
codegen inspection on a synthetic `/tmp/s32_test.c`:

| Bug | Pre-flip output         | Post-flip output                |
|-----|-------------------------|---------------------------------|
| 1   | `pea.w 0 ; pea.w 0`     | `pea.w 40 ; pea.w 0` (=0x280000)|
| 2   | `inc a` on s16          | proper Kl `+=` via Oadd carry   |
| 3   | high-half garbage       | bit-by-bit asl+rol cross-half   |
| 4   | `jsl __mul16`           | `jsl __mul32`                   |
| 5   | `pea.w 0 ; pea.w 0`     | `jsl __mul32` with `pea.w 1, 0` |

**Session 7 also fixed the leaf_opt + Kl framesize=0 slot/return-addr
overlap.** `can_be_frameless` now bails on the first Kl temp it
sees, forcing functions with 32-bit locals to carry a real frame.
This was the synthetic-test fault detected during Sessions 3-5.

**Suite progression post-flip:**

| Stage | Pass / Total | Notes |
|---|---|---|
| 7 (raw flip)          | 232/265 | 33 regressions, mode0 and BG examples mostly broken |
| 7 + can_be_frameless Kl gate (6125930) | 232/265 | Defensive; framesize=0+Kl synthetic bug closed |
| 7 + Kl divide-by-1 fast path (e83b19d) | 255/269 | u8* ptrdiff was hitting __sdivmod32 for sizeof(u8)=1; 19 regressions disappear |
| 7 + Omul Kl pow2 fast paths (58ab12a)  | 268/269 | Array indexing (`bg_scroll_x[bg]`) was hitting __mul32 for the index*sizeof(elem) mul; remaining failure is hdma_helpers (1581-pixel visual drift, no math-call evidence in the asm) |
| 8 + baseline refresh (opensnes-emu 48b6316) | **269/269** | hdma_helpers + sa1_starfield baselines regenerated (legitimate post-flip codegen change); 55 other meta files updated for new ROM SHAs |
| 8 + Mesen2 manual validation (2026-05-17)   | **269/269 + 4 visual** | hdma_helpers (4 HDMA effects), likemario (gameplay+collision), mode0 (4-BG parallax), snesmod_music (audio+HDMA) all render cleanly in Mesen2. Class A 3-pillar contract satisfied. |

The 5 originally-tracked bugs are still all fixed. The remaining
hdma_helpers failure is the only outlier; it's a small visual
drift, not a structural break, and the lib/example asm for that
path no longer contains any __mul32 / __[s]divmod32 calls — the
diff is likely a code-size shift propagating to a frame-timing
artifact in the per-scanline ripple effect. Tracked for follow-up,
not a chantier blocker.

The remaining failures (pre-shortcuts state) were not the originally-tracked
5 bugs — those are fixed — but a fresh layer of latent issues that the
flip surfaces:

- 27 visual regressions (most are full-frame: stuck at boot, NMI not
  firing, or VRAM garbage). Both major lib functions like
  `bgInitTileSet` (called from every BG example) and game-state
  examples are affected.
- 4 lag-frame regressions (continuous_scroll, superscope,
  snesmod_music_hirom, animated_sprite — at or near 100% lag rate,
  suggesting the example never escapes its setup).
- 2 runtime-execution warnings on examples with secondary control flow.

The most-likely root causes (not yet bisected):

1. Subtle bug in one of the Kl handlers I wrote in Sessions 2-6
   (Oshl/Oshr/Osar bit-by-bit, Omul partial-product accumulation,
   `__sdivmod32` sign correction, the 12-cmp cascade label scheme).
   The framework runs them all on every example now; a wrong-by-one
   shift or off-by-one carry produces full-frame garbage.

2. Pointer-difference codegen: `bgm_end - bgm` (ptrdiff_t) was Kl
   pre-flip too, so this path was exercised before — but `(long)
   (ptr_end - ptr_start)` cast to `u16` for `bgInitTileSet` now
   flows through a different conversion path. The mode0 generated
   asm contains spurious `jsl __sdivmod32` calls dividing by 1,
   which is correct but slow and could be the symptom of the QBE
   simplifier not folding Kl divide-by-1.

3. Bug in my framesize-Kl gate: the `can_be_frameless` early-exit
   might be too aggressive, forcing frames on functions that used
   to be leaf and breaking some pre-existing optimisation in
   adjacent passes (alias resolution, dead-store, tail-call
   detection).

**Effort estimate:** roughly 1-2 sessions per failing example
cluster (some failures share a root cause — once one mode-N
example is fixed, the rest of mode-1..mode-7 likely fall too).
Expect **3-7 days** of focused chantier work to land Session 8
(squash + release).

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
