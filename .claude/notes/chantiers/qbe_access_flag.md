# Chantier A9 — the access flag pins loads in QBE's optimiser

**Status:** DONE 2026-09-06 — both experiments clean, shipped as one commit on
`wip/qbe-access-flag` (from develop post-v0.39.0), squash-merged.
**Origin:** found during B2 Phase 2 (`b2_far_ram.md` §10b): a one-line
cproc change that stopped tainting pointer-variable loads with their
pointee's qualifiers changed the IR of 18 lib modules and broke three
examples at frame-equal comparison. Reverted; this is the follow-up.
**Risk:** High (optimiser + backend interplay). **Effort:** unknown until
Phase 0 says how many sites the exposure has.

## 1. The defect

cproc marks every load/store through a qualified lvalue with an access
flag: bit 0 `volat` (QUALVOLATILE, chantier A2), bit 1 `cst` (#121, the
object is const ROM — read with far addressing), bit 2 `farram` (B2, the
object is bank-$7E RAM). The w65816 backend keys its addressing on bits
1-2. But QBE's optimiser passes test the flag **as a whole**:

| Pass | Site | What it skips when `i->volat != 0` |
|---|---|---|
| `load.c` loadopt | `if (i->volat) continue;` (~l.440) | store→load and load→load forwarding |
| `mem.c` promote | `if (l->volat) goto Skip;` (~l.39) | promotion of an alloca to an SSA temp when any access to it is flagged |
| `gcm.c` canelim | `if (i->volat) return 0;` (~l.33) | elimination of an unused load |

So every const-data read (`table[i]`, `p->field` through a `const T *`),
every far access, and — because cproc's qualifier model puts the pointee's
qualifiers on the pointer type and `funcload` merged `e->type->qual` into
the access flag — every load of a `const T *` / `T FAR *` **pointer
variable**, is treated as volatile: never forwarded, never promoted out
of its alloca, never eliminated. Correctness is fine (over-pinning only
costs); performance is not: the lib is full of `const` pointer walks and
const tables.

## 2. Why it is not a one-liner

Unpinning is exactly what the reverted cproc change did for the
pointer-variable class, and it exposed something: with those loads
forwardable/promotable, 18 lib modules changed IR (allocas holding
pointers were promoted, `temp N: slot=-1 alloc=2`) and three examples
rendered differently — `hdma/gradient_colors`, `hdma/hdma_helpers`,
`basics/random` (before the HDMA mid-frame fix; to re-establish). The
backend, or a pass, mishandles a shape that pinning had been hiding.
Phase 0 must find which.

## 3. Phases

### Phase 0 — audit *(this)*
- Count the pinned accesses across the corpus IR (`CC65816_KEEP_IR`,
  added to the wrapper for this): per flag combination, loads vs stores,
  per TU. Separates "volatile, must stay" from "cst/farram, pinned by
  accident".
- Experiment A: unpin bits 1-2 in the three passes only (`i->volat & 1`).
  Rebuild the corpus, `make tests`, frame-equal comparison against the
  develop build (ROMs saved before the change). The failing set is the
  bug set; the codegen delta (`make bench`, instruction counts) is the
  prize.
- Experiment B (if A is clean or once its bugs are fixed): the cproc
  `accessqual()` change on top — pointer-variable loads lose the pointee
  taint entirely (no far path at all for `lda p`).
- For each failing example: luna state diff (PPU/DMA/WRAM) between the
  reference and the experiment ROM, then the IR diff of the TU that owns
  the differing state, then the emitted ASM. Name the shape.

### Phase 1 — the fix
Depends on Phase 0. Candidates: (a) a backend assumption that a Kl
pointer temp always has a stack slot (promoted allocas make the pointer a
pure SSA temp fed by phis); (b) loadopt forwarding a far load across a
store the backend considers aliasing; (c) `mark_dead_stores` /
`is_nop_instruction` invariants (the 2026-07-19 class).

### Phase 2 — ship
Split the flag in the three passes; cproc `accessqual()`; corpus green
with the frame-equal protocol; `make bench` re-baselined with the gain
stated; WRAM oracle updated.

## 4. Phase 0 outcome (2026-09-06)

### 4a. Counts (`CC65816_KEEP_IR`, 196 TUs incl. the lib's four builds and the compiler-test cases)

| flag | loads | stores |
|---|---|---|
| plain | 6995 | 6498 |
| `volat` (must stay pinned) | 201 | 1509 |
| `cst` (pinned by accident) | 285 | — |
| `farram` (pinned by accident) | 28 | 37 |

Lib alone (one build): 188 `cst` loads vs 65 `volat` loads — anim,
sprite, asset, panel, scene, sprite_dynamic_* carry most of them
(pointer walks through `const` clip/frame/metasprite tables).

### 4b. Experiment A — `i->volat & 1` in load.c / mem.c / gcm.c only

Everything else untouched (cproc still taints pointer-variable loads).
Rebuilt from clean; reference ROMs = the v0.39.0 build.

- Static: lib **−737 instructions (−4.58 %)**; cyclecount estimate per
  module: asset −34 %, anim −17 %, panel −15 %, sprite −8 %, background
  −7 %, collision −5.5 %, text −4 %, hdma −2.6 %, audio −1.4 %. The shape:
  allocas holding `const T *` walkers are promoted to SSA temps, so a
  loop like `*dest++ = *src++` drops the per-iteration alloc load/store
  round trips (breakout `mycopy` 57 → 45 instructions per iteration).
- Dynamic: `devtools/benchrom` is **identical** (its functions are ASM
  paths — mode7, map, dynamic sprites — with no pinned access); the
  "instructions to frame F" proxy is *inverted* for idle-heavy ROMs
  (luna counts the halted `wai` loop) and flat for CPU-bound ones. No
  existing bench measures this gain; the static numbers are the record.
- Correctness: `make test-compiler` 18/18, fixture 25/25, coverage
  83/85 unchanged, manifests 50/50, visual 81/85 with the 4 movers
  (basics_timer, games_shmup_1942, scrolling_parallax_scroll,
  sprites_metasprite) **matching the reference at frames 200 and 400**
  — pure animation-phase drift from fewer instructions per frame. WRAM
  oracle 74/85 drift, expected (codegen).
- The three examples that broke during B2's attempt (hdma/gradient_colors,
  hdma/hdma_helpers, basics/random) are **clean** here: their earlier
  failures were the HDMA mid-frame-enable glitch (fixed in v0.39.0) and
  the boot-time RNG seed, not an optimiser bug. The "latent bug" of §2
  does not exist under experiment A.

### 4c. Experiment B — cproc `accessqual()` on top of A

Pointer- and array-typed expressions contribute only their own `e->qual`
to the access flag (the pointee's qualifiers are picked up by the deref
in `mkunaryexpr`), so a `const T *p` / `T FAR *p` variable is loaded as
the bank-0 object it is (`lda.w p`, not `lda.l p` or the `[tcc__r9]`
path through a struct pointer). Same gates as A: test-compiler 18/18,
fixture 25/25, manifests 50/50, visual 81/85 with the **same four movers
and the same hashes as A** (frame-equal verified). Static: lib −685
instructions vs the reference (−4.25 %; +52 vs A — the pointer loads
are one `lda.w` instead of `lda.l`, cheaper per access, and a few
near-path sequences are longer than the far ones they replace).

### 4d. Verdict

The §2 "latent bug" never existed: the three examples that failed during
B2 were the HDMA mid-frame glitch and the RNG boot seed. Both
experiments ship together (Phase 1 and 2 collapse into one commit).

## 5. Decisions log

- 2026-09-06: chantier opened; Phase 0 started.
- 2026-09-06: Phase 0 done — A and B both clean; shipped as one commit.
