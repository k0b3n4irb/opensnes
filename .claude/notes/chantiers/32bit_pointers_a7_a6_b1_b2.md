# Chantier — Full 32-bit codegen + 24-bit pointers (A7 → A6 → B1/B2)

**Status:** PLANNED (2026-06-22) · **Owner:** TBD · **Risk:** High · **Effort:** ~6–10 weeks
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

## 7. Open questions for the maintainer

1. **One chantier or staged releases?** A7 alone (Phase 1) is shippable as a patch
   (finishes 32-bit math). A6+B1+B2 is the bigger minor. Ship A7 first, or hold all
   for one v0.22.0?
2. **Pointer size 4 vs 3 bytes** — the catalogue picks 4 (alignment sanity, dead
   high byte). Confirm before Phase 2 (it's expensive to change later).
3. **B2 emit:** per-symbol bank detection vs always-explicit-bank for RAM (simpler,
   ~1 cycle/access). Recommend always-explicit unless a hot-path benchmark objects.

---

**Cross-references:** `.claude/STRUCTURAL_DEFECTS.md` (A7/A6/B1/B2 detail),
`compiler/ABI.md` (type sizes / calling convention), `compiler/PINS.md` (QBE
patches), `.claude/rules/bank0_budget.md` (the ratchet), `KNOWN_LIMITATIONS.md`
(the two user-facing constraints this chantier lifts).
