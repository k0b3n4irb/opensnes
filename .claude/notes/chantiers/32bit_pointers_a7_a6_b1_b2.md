# Chantier — Full 32-bit codegen + 24-bit pointers (A7 → A6 → B1/B2)

**Status:** A7 SCOPED (2026-06-22) — immediate scope is **A7 only** (Phases 0–1,
ships as a patch); A6/B1/B2 (Phases 2–4) deferred to a follow-up chantier. See §7.
· **Owner:** TBD · **Risk:** High (A7 alone: Medium) · **Effort:** A7 ≈ 2 weeks; full thread ~6–10
**Catalogue entries:** `.claude/STRUCTURAL_DEFECTS.md` A7, A6, B1, B2 (per-item
symptom/root-cause/acceptance live there; this doc is the *sequencing & execution*
plan that ties them into one chantier).

---

## 1. Thesis (why this order, why one chantier)

The four items are one structural thread, not four independent fixes:

- **A7** = QBE w65816 only half-implements the 32-bit (`Kl`) instruction class.
- **A6** = pointers are mis-sized in the IR and indirect calls hardcode bank `$00`.
- **B1/B2** = the *user-visible symptoms* — "assets must live in bank `$00`" and
  "all C RAM must live below `$2000`" — are downstream of A6/A7.

Two facts from the catalogue drive the plan:

1. **A6 + A7 share the slot allocator.** Both need QBE temps to allocate **4-byte
   slots** instead of today's 2-byte default (`compiler/qbe/w65816/abi.c`). Doing
   them as one chantier means widening the allocator **once**.
2. **A6 subsumes B1/B3/B4.** A6's acceptance says: once the compiler emits proper
   24-bit addressing, the lib no longer needs hand-written `*Bank` API variants —
   bank-agnostic data "just works". So **we deliberately do NOT take the lib-led
   path** (adding `*Bank` variants now would be throwaway work). B1 collapses from
   "add 12–15 `*Bank` functions" to "verify subsumed + lift the constraint +
   tighten the ratchet + update docs". Likewise B2 becomes a compiler emit change,
   not a lib change.

**Therefore: compiler-first.** Finish A7, land A6 on the shared slot work, then
B2 (RAM emit) and B1 (lift the bank-`$00` constraint) fall out.

## 2. Current state — A7 is partially shipped (do NOT treat as greenfield)

The **fix32 chantier (B5, v0.21.0)** already landed part of A7:
- the **`Kl` return convention** (32-bit results returned from C functions),
- `__mul32` runtime (`Kl` `Omul`), with the bank-byte-drop bug fixed,
- `Kl` shift-by-constant codegen (the `low_orig` spill fix),
- a 48-iteration long divide (used by `fix32Div`).

So A7's items 1 (slot), 3 (add/sub), 4 (shift), 6 (mul) are **at least partially**
present. **Phase 0 must audit exactly which `Kl` ops are implemented and correct**
before writing new code — the plan's Phase 1 fills gaps, it does not start from zero.

## 3. Phases & gates

Each phase ends with a **gate**: `make clean && make` + `make tests` (luna) 56/56 +
the phase's own runtime-correctness fixtures green. No phase merges to develop until
its gate passes. Use a `wip/32bit-*` branch per phase (squash-merge per the release
rules).

### Phase 0 — Audit + test scaffold *(~3 days)*
- Enumerate every `Kl` opcode path in `compiler/qbe/w65816/{abi.c,emit.c}`; mark
  implemented / partial / missing against A7's list (load/store, add/sub, shl/shr/sar,
  neg/not/xor/and/or, mul, div/mod, comparisons).
- **Build the runtime-correctness harness on luna** (the catalogue still references
  the removed opensnes-emu/snes9x bridge — replace with luna). New fixtures under
  `devtools/compiler-tests/` *or* a small ROM that computes the cases and luna
  asserts via `--assert` on WRAM (we already have this pattern in the probes).
  Cases: `0xFFFF+1 == 0x10000`, `(s32)-1 >> 8`, `u32` mul/div, all comparisons,
  shifts of arbitrary amounts, pointer round-trips. **This is the gate for every
  later phase** — the existing ASM-pattern checks would pass a half-fixed impl.

### Phase 1 — A7: finish `Kl` codegen *(~1.5–2 weeks)*
- **Slot allocator**: `Kl` temps → 4-byte slots (`abi.c`). This is the shared piece
  A6 reuses — get it right here.
- Fill the missing/partial `Kl` ops from the Phase-0 audit (load/store pairs,
  add/sub carry chain, shift pair w/ shift-by-16 swap, neg/logical pairs,
  div/mod via `__sdiv32`/`__udiv32`, two-half comparisons).
- Gate: runtime fixtures (Phase 0) all green; `fixDiv`/`fixLerp` re-validated;
  `compiler/ABI.md` type-size table updated; `compiler/PINS.md` records new QBE
  patches.

### Phase 2 — A6: 24-bit pointers + indirect-call bank byte *(~2–3 weeks)*
- **cproc**: `mkpointertype()` → `size=4, align=2` (bytes 0–2 = 24-bit address,
  byte 3 = dead padding). `compiler/cproc/type.c`.
- **emit**: rewrite the indirect-call sequence (`compiler/qbe/w65816/emit.c`) to
  read the bank byte from pointer storage (`lda 2,x → tcc__r9+2`) instead of the
  hardcoded `lda #$00`; reuse the Phase-1 4-byte slots.
- **static data**: pointer fields emit `.dl symbol` + 1 byte padding.
- **Audits (the risky part):** the 28 QBE patches that read pointer offsets;
  `templates/crt0.asm` indirect-call sites (`nmi_callback`, `dynamic_flush_hook`);
  lib function-pointer setters (`nmiSet`, `sceneRun`, `gameLoopRun`,
  `oamInitDynamicSprite`).
- Gate: `sizeof(void*)==4`; all SA-1/SuperFX examples pass luna visual + the
  framework examples (scene_stack, gameloop) green; **C.5 padding workaround
  reverted** (its raison d'être disappears); `make tests` 56/56.

### Phase 3 — B2: C RAM outside `$2000` *(~2 weeks, partly overlaps Phase 2)*
- emit explicit 24-bit addressing for RAM symbols whose bank ≠ `$00` (`sta.l
  $7E:NNNN,x`), or always-explicit-bank for RAM accepting the ~1-cycle cost.
- linker / `memmap*.inc`: allow RAM allocation above the 8 KB band (keep
  `$00:0000–$1FFF` recommended for hot data).
- Gate: a regression fixture puts a struct at `$7E:2500` and round-trips r/w under
  luna; all 56 examples still pass.

### Phase 4 — B1: lift the bank-`$00` constraint (now mostly verification) *(~1 week)*
- Confirm A6 subsumes the lib `*Bank` need: relocate a large asset to a non-`$00`
  bank in one of the tight examples and verify it renders (no `*Bank` call).
- Tighten `BANK0_FAIL_THRESHOLD` (target 256) now that the tight examples have
  real headroom; update `.claude/rules/bank0_budget.md`.
- Update `KNOWN_LIMITATIONS.md` (severity drops on the bank-`$00` and RAM-`$2000`
  entries); close **B1/B3/B4 in the catalogue as "subsumed by A6"**, mark B2 done.

## 3b. Phase 0 OUTCOME (2026-06-22) — A7 is far more done than the catalogue says

**Audit.** The QBE w65816 backend already implements the `Kl` class broadly —
`Oload`/`Ostore`/`Ostorel`, `Oadd`/`Osub` (carry chains), `Omul` (`__mul32`),
`Odiv`/`Oudiv`/`Orem`/`Ourem` (`tcc_sdivmod32`/`tcc_udivmod32`), `Oshl`/`Oshr`/
`Osar` (pair sequences), `Oneg`/`Oand`/`Oor`/`Oxor`, comparisons, plus high-half
dead/zero optimization pre-passes. The catalogue's premise ("never extended past
16-bit", "NOT yet attempted") is **stale** — prior chantiers (incl. fix32 v0.21.0)
did the bulk. **A7's remaining work is verification + edge-case fixes, not a
from-scratch implementation.**

**Runtime harness built** — `devtools/compiler-tests/runtime/a7_32bit/`
(`main.c` + `Makefile` + `test_a7_32bit.py`): a ROM computing 13 s32/u32 ops into
WRAM globals, checked via `luna state --assert`. Run:
`cd devtools/compiler-tests/runtime/a7_32bit && make && python3 test_a7_32bit.py`.
**Result: 12/13 correct, 1 real bug (xfail'd).** This is the permanent A7 gate —
the static C→ASM checks pass even on the broken case, so only this catches it.

**The one real bug (= A7 Phase 1 scope):**
`(s32)-256 >> 4` folds to `0x0FFFFFF0` instead of `0xFFFFFFF0`. Root cause is
**not the codegen** (the `Osar Kl` emit path looks correct) but the **constant
folder**: `compiler/qbe/fold.c:80`
`case Osar: x = (w ? l.s : (int32_t)l.s) >> (r.u & (31|w<<5));`
On w65816 `Kl` is **32-bit**, but QBE folds it as 64-bit (`w==1` → `l.s`,
64-bit). A negative 32-bit constant is stored in the 64-bit `con` **without
sign-extension** (`0x00000000FFFFFF00`), so the 64-bit arithmetic `>>` fills from
bit 63 (=0). `Oshr` (logical) survives because logical-shifting a zero-extended
value gives the right low-32; only sign-dependent folds break.

**Latent siblings (verify in Phase 1):** the same non-sign-extended-con issue
affects **signed `Kl` divide/mod and signed comparisons** on negative 32-bit
constants — add cases to the harness before fixing.

**Phase 1 fix options (decide before coding — Class A, fold pass is delicate):**
- **(A) sign-extend 32-bit constants into the 64-bit `con` at materialization** —
  fixes Osar + signed div/cmp at the source, keeps `fold.c` generic. More
  invasive (find the Kl-constant build path); broadest correctness.
- **(B) make `fold.c` treat `Kl` as 32-bit on this fork** (`(int32_t)l.s` for
  Osar, mask others to 32-bit) — localized, matches the backend's Kl=32-bit
  reality, but touches generic QBE core and must be swept across every Kl fold.
- Either way: extend the harness (signed div/cmp), then fix, then gate green,
  then revisit B5's "blocked by A7" note (already shipped) and ship A7 as a patch.

## 3c. Phase 1 DONE (2026-06-22) — A7 fixed

**Blast radius (measured by extending the harness with signed div/mod/compare on
negative 32-bit constants):** narrower than feared. The asm proved signed
`Kl` div/mod take the **runtime** path (`tcc_sdivmod32`, not folded) and signed
compares are branch-coded — both correct. **Only the `Osar` constant-fold was
broken** (two cases: `>>4` and `>>8`).

**Fix:** `compiler/qbe/fold.c` `case Osar` now uses 32-bit signed semantics
(`(int32_t)l.s >> (r.u & 31)`) — correct for w65816 where `Kl` is 32-bit and there
is no 64-bit integer. One-line change, commented for a future QBE rebase. qbe
fork commit `1884a20` on `fix/a6-a7-leaf-opt-kl-frameless`; `compiler/PINS.md`
bumped `e50f148 → 1884a20` (50 patches); `make verify-toolchain` green.

**Validation:** the runtime harness is now **18/18** (no xfail) and is a permanent
gate — wired into `make tests` and the CI `functional-tests` job. Full Class-A
re-validation: `make clean && make` (compiler + lib + 56 examples) + `make tests`
→ compiler checks 10/10, coverage 54 OK / 2 INPUT-DEP, **visual 56/56 (no fbhash
drift)**, probes 10/10. Ships as a patch (v0.21.2).

**Deferred (unchanged):** A6 / B1 / B2 (Phases 2–4) remain a follow-up chantier;
decisions #2 (pointer = 4 bytes) and #3 (B2 per-symbol bank) pre-recorded above.

## 3d. A6 Phase 0 OUTCOME (2026-06-22) — the gap is the data deref, not the pointer

Audit of the actual backend state (same method as A7 — the catalogue was stale):

- **Pointer SIZE: already done.** `compiler/cproc/type.c` `mkpointertype()` is
  `t->size = 4, t->align = 2` — exactly A6's proposed fix (the catalogue's
  `size = 8` is stale). cproc already lays out 4-byte (24-bit + pad) pointers.
- **Indirect CALLS (function pointers): handled.** The A6+A7 backend commit
  (qbe `179676e`) added "full pointer ABI + Kl pair lowering" incl. the indirect
  call reading the bank byte from pointer storage; the framework examples
  (scene_stack, gameloop, nmiSet callbacks) run green, exercising it.
- **THE REMAINING GAP — data load/store through a pointer is bank-`$00`-hardcoded.**
  `compiler/qbe/w65816/emit.c` (explicitly documented at lines 533–543) lowers
  `*ptr` loads/stores as `lda.l $0000,x` / `sta.l $0000,x` (sites 3482/3519/3552/
  3617): X = the low 16 bits of the address, **bank fixed to $00**, the pointer's
  bank byte ignored. There's even a `temp_addr_only` optimization that drops the
  high-half (bank) computation *because* it's dead at every deref consumer. **This
  is the real B1/B2 root cause:** `*ptr` to data outside bank `$00` reads garbage.

### Phase 1 scope (now precise)
1. Lower pointer load/store as a **24-bit deref using the bank byte** — the
   65816 way is direct-page indirect long (`lda [dp],y` / `sta [dp],y`), reading
   the full 3-byte pointer from a DP slot, instead of `lda.l $0000,x`. Touches
   the 4 `$0000,x` sites + their load counterparts.
2. **Retire / gate the `temp_addr_only` optimization** — it assumes the bank byte
   is dead; it must only fire when the pointer is provably bank-`$00`, else the
   bank byte is now live.
3. Decision #3 (per-symbol bank) applies to **named RAM symbols** (B2); arbitrary
   pointer derefs (this item) must always carry the bank since the bank isn't
   known at compile time.

### Runtime harness — needs cross-bank placement (Phase 1 infra)
Unlike A7 (pure arithmetic), proving this needs **data physically outside bank
`$00`**, which fights the bank-`$00`-overflow guard. Approach: a fixture that
places a known sentinel array in a high bank via a `.section … BANK N` directive
(asm or linker), takes a pointer to it, derefs through the pointer, and luna
`--assert`s the value. Build this first in Phase 1 (it's the gate), then fix the
codegen.

**Net:** A6 is ~half done (pointer size + calls); the hard, high-value half (data
deref → lifts B1/B2) remains and is now precisely scoped.

### Phase 1 — runtime gate built + bug proven (2026-06-22)
`devtools/compiler-tests/runtime/a6_farptr/` puts a sentinel in **BANK 2**
(`a6_sentinel.asm`, lands at `02:8000`), takes a **volatile** far pointer to it
(forces a register-indirect deref the compiler can't fold to a direct access),
and luna-`--assert`s the results. Outcome, isolating storage vs deref:
- **`r_ptrhi == 0x0002` PASS** — the pointer *value* keeps its bank byte (storage
  is correct).
- **`r_d0/d3/d7` FAIL** (xfail) — the deref reads bank `$00` garbage (0x08/0xe2/
  0x8d at `$00:8000+`) instead of 0x11/0x44/0x88. The gap is the **deref only**.

So the bug is proven and isolated. The harness is committed with the 3 deref
cases xfail'd (not yet wired into `make tests`/CI — flips green when fixed).

### Phase 1 — the codegen design decision (maintainer call needed)
Each deref site (`emit.c` ~3482/3519/3552/3617 + load counterparts) is
`tax … lda.l/sta.l $0000,x` — X = the pointer's low 16, bank fixed `$00`. The
65816 has no "store to (X + runtime bank)"; the fix is **direct-page indirect
long** (`lda [dp],y` / `sta [dp],y`) after writing the 3-byte pointer to a DP
scratch. That costs ~+2–5 cyc **plus** ~3 stores to set up the DP pointer, **per
deref**. Two ways:
- **(F) always-far** — every pointer deref uses `[dp],y`. Simple, but a real perf
  regression on the ~99% bank-`$00` case (every array index / C-RAM access).
- **(S) bank-0 fast path** — keep `lda.l $0000,x` when the pointer is *provably*
  bank `$00` (C RAM, `&local`, bank-0 globals), use `[dp],y` only for
  possibly-far pointers. Preserves perf; needs a "provably bank-0" dataflow
  analysis (related to the existing `mark_addr_only_kl`/`temp_addr_only` machinery,
  which already tracks "bank byte dead"). More complex, higher risk.

**Recommendation: (S)** — a game SDK can't eat a per-access cycle tax on every
array index. (S) is multi-day and must be done carefully (a wrong bank-0 proof =
silent corruption). This is the decision to confirm before the codegen work.

### DECIDED: (S). Implementation spec (2026-06-22)

Design completed + de-risked; the codegen is a focused multi-day execution.

**Safety principle (why (S) won't corrupt):** default every deref to the FAR path;
use the fast bank-`$00` path ONLY when the address is *provably* high-zero via the
existing `ref_is_high_zero()` (emit.c:495). A genuinely-far pointer is never
high-zero-provable (its base is a `loadl`/non-zero value), so it always takes the
far path → correct. The only "fast on non-zero" risk is a low-16 add that carries
past `$FFFF` into the bank — which the *current* hardcoded-`$0000,x` codegen
already gets wrong (data must fit in its bank); (S) introduces no NEW unsoundness.

**Three pieces:**

1. **Deref dispatch.** At each `$0000,x` load/store site, branch on
   `ref_is_high_zero(addr, fn)`:
   - true  → keep the current fast path (`lda.l/sta.l $0000,x`, or `lda.w sym` for
     a CAddr operand);
   - false → far path (below).
   Sites: stores `emit.c` ~3482/3519/3552/3617, loads ~3741/3769 (+ the
   `Oloadsw/Oloaduw/Oload` and `Oloadsb/Oloadub` / half variants).

2. **Keep `arr[i]` fast — extend `mark_high_zero()`** (emit.c:466). Today it marks
   only `Oextuw` temps; `buf[i]` lowers to `add $buf, extuw(idx)` whose *result*
   isn't marked → would wrongly take the far path (perf regression). Extend the
   pre-pass to also mark `Oadd`/`Osub` Kl temps when **both** operands are
   high-zero (RCon high16==0, Kw class, or already-marked), to a fixpoint. Sound
   under the same no-bank-wrap assumption the fast path already relies on.

3. **Far-path codegen** (byte-load shape; mirror for word/half + stores): write the
   3-byte pointer to a DP scratch, then DP-indirect-long:
   ```
   emitload(r0, fn);            ; A = ptr low16
   sta.b  <DP>                  ; DP,DP+1 = low16
   emit_load_high(r0, fn, 0);   ; A = ptr high16 (bank in low byte)
   sep #$20
   sta.b  <DP+2>                ; DP+2 = bank byte
   lda    [<DP>]                ; 24-bit load (sep for byte; rep+lda [DP] for word)
   rep #$20
   ```
   **KEY OPEN RISK — the DP scratch.** `lda [dp]` needs 3 contiguous direct-page
   bytes that are FREE at the deref site (not a live `tcc__rN`). Options: reserve a
   dedicated 3-byte deref scratch in the runtime DP map (`templates`/runtime DP
   allocation), or prove a `tcc__rN` dead here. This allocation (without clobbering
   a live temp across all variants) is the crux of the remaining work — nail it
   before writing the emit code.

**Validation gates (all required before commit — Class A):**
- `devtools/compiler-tests/runtime/a6_farptr/` flips green (remove the 3 xfails;
  far deref now correct) AND a new far-STORE case added.
- `luna_runner.py --compare` 56/56 (bank-`$00` fast path preserved → no fbhash drift).
- `make bench` no cycle regression (the `mark_high_zero` Oadd extension keeps
  `arr[i]` on the fast path).
- `make clean && make` + full `make tests`.
- Add far-pointer cases that exercise stores, struct fields, and `p[i]` (indexed).

### Phase 1 — implementation ATTEMPT #1 (2026-06-22) — regressed, NOT shipped

Built: DP scratch `tcc__farptr` (crt0, reserved + NMI-mirrored, validated inert),
far **byte-load** path (DP-indirect-long via `tcc__farptr`), `mark_addr_only_kl`
gated on `ref_is_high_zero` (so a far pointer's bank stays live), and
`mark_high_zero` extended to propagate through `Oadd Kl` (+ treat CAddr as bank-0).

- **Isolated far byte-load: PROVEN correct** — `a6_farptr` r_d0/d3/d7 XPASS (the
  mechanism works: addr_only gating + DP-indirect-long deref).
- **Full validation FAILED** → not shipped:
  - **7 visual regressions**: aim_target, random, timer, breakout, tetris,
    metasprite, superscope (fbhash drift = wrong codegen at scale).
  - **+8% cycles** (bench 1476→1595): bank-0 derefs not proven high-zero fell to
    the slower far path.

**Lessons for attempt #2:**
1. Only the **byte-load** far path was implemented; **stores + word/half loads**
   still use `$0000,x`, so making their address temps non-addr-only (high half
   computed) without a matching far emit is incoherent — implement ALL deref
   sizes together, or none.
2. The `mark_high_zero` Oadd/CAddr extension is too coarse and/or the far path
   has edge cases — the 7 regressions must be root-caused individually (likely
   far STOREs and far word-loads hitting the unimplemented sites, plus the
   perf fall-through). Bisect per-example with `CC_TRACE`/asm diff.
3. The +8% confirms the high-zero analysis under-approximates real bank-0
   derefs — it must prove high-zero for ~all current patterns before the far
   default is acceptable.

WIP kept on branch `chantier/a6-codegen` (NOT merged — develop stays correct).
The gate (`a6_farptr`) + this design remain on develop for attempt #2.

## 4. Test strategy (luna, not the removed bridge)

The catalogue's acceptance criteria predate the luna migration and reference
`tools/opensnes-emu/test/fixtures/runtime/` + "the snes9x bridge". **Re-home all of
that onto luna**: runtime-correctness = a ROM that computes results into WRAM +
`luna state --assert BANK:OFF=HEX` (exactly the `probes/` pattern). This becomes the
permanent regression gate so 32-bit codegen can't silently rot — the single most
important deliverable of Phase 0.

## 5. Risk & rollback

- **Highest-risk area:** the QBE slot allocator + emit pass — the same code that
  carried the C.5 padding bug and the `__mul32` bank-byte bug. Every QBE patch must
  be re-validated against 4-byte slots.
- **Rollback:** each phase is its own `wip/` branch; develop only advances on a
  green gate. The `Kl`/pointer changes are compiler-internal — a bad phase is
  reverted by not merging, not by touching shipped ROMs.
- **Cross-arch:** keep the visual gate on `--print-fbhash` (cross-arch stable); the
  new runtime fixtures assert WRAM values (deterministic), not raw WRAM hashes (see
  the H7 cross-arch caveat — `wram_regress` is local-only).

## 6. Definition of done

- A7: all `Kl` ops correct, runtime fixtures green, ABI/PINS updated.
- A6: `sizeof(void*)==4`, indirect calls cross banks, C.5 reverted, framework +
  chip examples green.
- B2: C variables work above `$2000` (luna round-trip fixture).
- B1: a tight example ships a large asset outside bank `$00` with no `*Bank` call;
  ratchet tightened; KNOWN_LIMITATIONS severities dropped; B1/B3/B4 closed as
  subsumed.
- One CHANGELOG section (likely a minor bump, e.g. v0.22.0) covering the unblock:
  "assets and C RAM are no longer confined to bank `$00` / below `$2000`."

## 7. Decisions (maintainer, 2026-06-22)

1. **Scope: A7 ONLY for now.** Do Phase 0 + Phase 1 (finish 32-bit `Kl` codegen)
   and ship it as a **patch** release. **A6, B1, B2 are deferred** to a separate
   follow-up chantier (a later minor, e.g. v0.22.0) — this doc keeps their phases
   (2–4) as the pre-written plan for when that chantier opens. Immediate scope
   below collapses to Phases 0–1.

2. **Pointer size = 4 bytes** (decided for A6, not needed for A7). Rationale: the
   65816 has no 3-byte load — a pointer is always read as a 16-bit word + a bank
   byte, so 3 vs 4 doesn't change access cost; 4 aligns with `long`=4 (post-A1) so
   mixed int/long/pointer structs stay aligned and the C.5 padding bug class stays
   closed; the only cost is one dead byte/pointer (negligible outside large pointer
   arrays). Revisit only if a pointer-array-heavy ROM shows real bloat.

3. **B2 emit = per-symbol bank detection** (decided for B2, not needed for A7).
   Rationale: a 24-bit RAM access costs ~1 cycle + 1 byte more than the implicit
   bank-`$00` form; *always-explicit* would tax **every** RAM access (incl. the
   ~99% that stay in bank `$00`) → a global perf regression. Per-symbol keeps the
   cheap form for bank-`$00` symbols and only pays the 24-bit tax for data
   deliberately placed high. Fall back to always-explicit only if per-symbol bank
   tracking proves too invasive in QBE.

### Immediate scope (this chantier) = Phases 0–1 only

- **Phase 0** — audit current `Kl` op coverage + build the luna runtime-correctness
  harness (the permanent gate).
- **Phase 1** — finish the `Kl` codegen (4-byte slot allocator + remaining ops +
  `__mul32`/`__udiv32`/`__sdiv32` gaps), runtime fixtures green, ABI/PINS updated.
- **Ship**: a patch release ("32-bit `s32`/`u32` arithmetic is now correct
  end-to-end"). The slot-allocator widening done here is the shared piece the
  deferred A6 will reuse.

Phases 2–4 (A6 / B2 / B1) remain documented above but are **out of scope** until
the follow-up chantier opens; decisions #2 and #3 are pre-recorded for it.

---

**Cross-references:** `.claude/STRUCTURAL_DEFECTS.md` (A7/A6/B1/B2 detail),
`compiler/ABI.md` (type sizes / calling convention), `compiler/PINS.md` (QBE
patches), `.claude/rules/bank0_budget.md` (the ratchet), `KNOWN_LIMITATIONS.md`
(the two user-facing constraints this chantier lifts).
