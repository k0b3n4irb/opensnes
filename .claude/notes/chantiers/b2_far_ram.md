# Chantier B2 — C RAM beyond the 8 KB band (`__far` objects in bank $7E)

**Status:** PLANNED (2026-09-05), not started. Branch `wip/b2-far-ram`,
squash-merge as `feat(compiler,runtime,lib,docs): …`, MINOR release.
**Catalogue entry:** `.claude/STRUCTURAL_DEFECTS.md` §B2 (🔴). This note is
the execution plan; the catalogue keeps symptom / acceptance.
**Risk:** High (codegen + runtime). **Effort:** ≈ 3 weeks.

## 1. Why now

The C RAM band is `$00:0000–$1FFF`: 8 KB shared by the stack (top at
`$1FFF`), the direct-page register file, every lib RAMSECTION in bank 0 and
every C global. The link-time instrument (`symmap.py --check-ram-budget`,
2026-07-11) turned the ceiling into a number, and the number is the decision
gate the catalogue set. Corpus, 2026-09-05:

| Example | Free bytes | What sits there |
|---|---|---|
| games/breakout | 1436 | `.game_buffers` 4708 B |
| games/tetris | 1696 | |
| mode7/dsp1_ground | 1744 | 2 KB of double-buffered HDMA tables |
| sprites/dynamic_metasprite | 2016 | |
| games/rpg (the showcase) | 2784 | state still to add |

The SNES has 128 KB of WRAM. The lib already uses bank $7E freely (map.asm
parks ~10 KB in `.RAMSECTION ".map_bank7e" BANK $7E SLOT 2`, lzss too); C
cannot. That is the gap: **a C object above `$1FFF` is silently
wrong-banked**, so the templates forbid it.

## 2. What the compiler does today (audited 2026-09-05 — do not trust the catalogue's "never")

The catalogue's root cause ("the emit pass doesn't track banks") is half
stale. Since #121 the backend already has a complete *far read* path, keyed
on a taint bit cproc sets per access:

- `compiler/cproc/qbe.c:737` — `is_volat = (QUALVOLATILE ? 1 : 0) |
  (QUALCONST ? 2 : 0)`; bit 1 means "the object's bank is not known to be
  $00", carried on the QBE instruction as `volat & 2`.
- `compiler/qbe/w65816/emit.c` honours it on **loads** in three forms:
  symbol-direct `lda.l sym[+off]` (the linker supplies the 24-bit address,
  line 3958 / 4002), runtime pointer `emit_cst_ptr_setup` + `lda [tcc__r9]`
  (line 3962 / 4007), and the **indexed-long fusion** `add($sym, idx)` +
  load → `emitload(idx); tax; lda.l sym,x` (line 937 / 4857).
- Everything **not** tainted is bank-blind: `lda.l $0000,x` / `sta.l $0000,x`
  (four store sites 3715–3850, loads 3987 / 4042), and `Ostorel` uses
  `sta.l $0002,x` for the high half. `temp_addr_only` (line 387) even skips
  computing the bank byte of address temps *because* every consumer drops
  it. **Stores have no far path at all** — const data is never written.
- Section placement: `compiler/qbe/emit.c:159/293` emits every C global as
  `.RAMSECTION ".bss.N" BANK 0 SLOT 1`; initialised globals get a
  `.data_init.N` record `{addr16, size16, bytes}` that `CopyInitData`
  (`templates/crt0.asm:738`) replays with `(tcc__r2),y` — 16-bit indirect,
  DBR = 0, so it can only land in bank 0. crt0 zero-fills only `$0000–$1FFF`.
- Pointers are 4-byte, 24-bit-in-storage since A6 (`mkpointertype` size 4):
  a pointer *to* far RAM already carries the right bank byte; only the
  deref ignores it. Lib ASM that takes far pointers post-A6 (`dmaCopyVram`,
  `hdmaSetup`, `dsp1Raster`, …) reads the bank byte and therefore already
  works with bank-$7E buffers.
- Stack locals (`lda N,s`, `[tcc__fp],y`) live in bank 0 by construction
  and are untouched by this chantier.
- Memory map: `templates/memmap.inc` already defines `SLOT 2 $2000 $E000`
  and the lib uses it as `BANK $7E SLOT 2`. No wla-dx change needed.

So B2 is not "add bank tracking"; it is **generalise the #121 taint to a
second address space (far RAM), extend the far path to stores and the
32-bit class, and place + initialise the objects**.

## 3. Design decision (maintainer call before Phase 1)

**(A) Opt-in `__far` objects — recommended.** A C object declared far lives
in bank $7E; every access to it, direct or through a pointer to a
far-qualified type, takes the far path. Everything else keeps today's
codegen and cost. This mirrors exactly what `const` does since #121, with
the same propagation rules (the qualifier rides on the pointee type, so
`u8 __far *p` derefs are far; casting it away is the user's silent-failure
button, as with const → we warn, see Phase 1).

**(B) Far-by-default for runtime pointers.** Keep symbol-direct and
`sym[idx]` accesses bank-exact (the linker knows the bank), and make every
runtime-pointer deref honour the bank byte. No qualifier, any object
anywhere. Cost: the C1 audit measured the far deref path at **+92 %** on a
pointer-walking probe (`lda [tcc__r9]` after staging the pointer in DP vs
`tax; lda.l $0000,x`). Unacceptable as a default while "30 % faster than
PVSnesLib" is the headline; revisit once the far deref is optimised (keep
the pointer resident in a DP pair across a loop — a register-allocation
change, out of scope here).

Spelling of the qualifier: `__far` keyword (cproc gets a new type qualifier
bit `QUALFAR`) exposed as `FAR` in `<snes/types.h>` — not `__attribute__`,
because the qualifier must be part of the *type* to propagate through
pointers, and cproc's attribute parser attaches to declarators, not types.
`__far` on a `const` object is rejected (ROM is already far).

## 4. Phases & gates

Each phase ends with `make clean && make`, `make tests` (luna) and
`wram_regress.py` green on the whole corpus; codegen drift is justified in
the commit as the rules require. Runtime gate fixture for the whole
chantier: `devtools/compiler-tests/runtime/b2_far_ram/` (same shape as
`a7_32bit/`): a ROM that declares `__far` scalars, an array, a struct,
an initialised far array and a far pointer walk, does read/write round
trips at `$7E:2xxx`, parks results in bank-0 WRAM, checked by
`luna state --assert`.

### Phase 0 — audit + fixture *(2 days)*
- Write the fixture first, against today's compiler: it must **fail** in a
  way that names the bank-blind site (place one object above `$1FFF` by hand
  with a `.RAMSECTION BANK $7E SLOT 2` in an ASM file and read it from C).
- Enumerate every emit site that assumes bank 0 for a *runtime* address:
  the four `sta.l $0000,x`, the two `lda.l $0000,x`, `Ostorel`'s
  `$0002,x`, `temp_addr_only`'s premise, and the Kl second-half load path
  (excluded from `temp_addr_only` for a reason — read it).
- List the lib ASM functions that take a *plain* pointer and deref it with
  16-bit addressing (`lda.w`/`lda (dp),y` with DBR = 0). Candidates: the 416
  `.w sym` accesses are mostly lib-internal state (oambuffer, f32_res_*),
  fine; what matters is pointer *parameters*. Any hit becomes a Phase 3 item.

### Phase 1 — cproc: the `__far` qualifier *(3 days)*
- `cc.h`: `QUALFAR = 1<<5`; lexer keyword `__far`; parsed with the other
  qualifiers; participates in type compatibility like `const` (assigning a
  `T __far *` to a `T *` warns "discards far"; the reverse is fine — a
  far-qualified pointer to a bank-0 object just takes the slower path,
  correctly).
- `qbe.c:737`: `is_volat |= (tq & QUALFAR) ? 4 : 0` on loads **and stores**
  (today stores never get a taint). Data emission: far globals get a
  distinct data kind so the backend can section them.
- Diagnostics: `__far` + `const` is an error; `__far` on a local is an error
  (locals are stack).

### Phase 2 — QBE w65816: far RAM as a second address space *(1.5 weeks)*
- Loads: `volat & 4` takes the three #121 forms unchanged (they are already
  bank-agnostic). Fold the two taints into one predicate `is_far(i)`.
- **Stores** (new): symbol-direct `sta.l sym[+off]`; indexed `sta.l sym,x`
  when the address is `add($sym, idx)` (mirror the load fusion at 4857);
  runtime pointer `emit_cst_ptr_setup` + `sta [tcc__r9]` / `,y`. Byte
  stores under `sep #$20`. Kl (`Ostorel`, 32-bit loads): both halves through
  the same base, `+2`.
- `temp_addr_only`: a far-tainted consumer *does* use the bank byte, so the
  pre-pass must exclude those temps (or the address temp's high half is
  garbage exactly when it matters). Same for `is_high_dead_propagating`.
- Sections: far globals → `.RAMSECTION ".far.N" BANK $7E SLOT 2` (bss) and
  the init record gains a bank byte: `{addr16, bank8, pad8, size16, bytes}`
  — `CopyInitData` switches the destination to `[tcc__r2],y` with the bank
  in `tcc__r2+2` (the source stays 16-bit: `.data_init` is SEMIFREE in bank
  0). Bank-0 records carry bank 0, so one loop serves both.
- Zero-fill: crt0 clears `$7E:2000–$FFFF` at boot with a fixed-source DMA
  to `$2180` (≈ 56 KB, a few ms, before any lib init). map.asm's bank-$7E
  state is initialised by `mapLoad` anyway; confirm with the maps examples.
- The symmap RAM budget check learns a second band: report bank-$7E far
  usage next to the bank-0 number (no fail threshold yet).

### Phase 3 — lib + corpus *(3 days)*
- Fix every Phase 0 lib hit (pointer parameters deref'd bank-blind), with
  the ABI lint's `lda N,s` annotations as the map.
- Migrate `games/breakout`'s `.game_buffers` (4708 B) to `__far` — the
  showcase and the regression test that matters: pixel-identical fbhash,
  probes green, bank-0 free bytes go from 1436 to ≈ 6100.
- Optionally `mode7/dsp1_ground`'s HDMA tables (they are consumed by HDMA
  from any bank; the far pointer already carries `$7E`).

### Phase 4 — docs, ratchet, catalogue *(2 days)*
- `KNOWN_LIMITATIONS.md`: the 🔴 entry becomes 🟡 "bank-0 band is the
  default; `FAR` moves an object to bank $7E" with the cost table
  (symbol-direct: free; `sym[idx]`: free; runtime pointer deref: +N cycles,
  measured). `compiler/ABI.md` gains the address-space section.
- `docs/tutorials/memory.md` (or the closest): when to use `FAR` (bulk
  buffers, HDMA tables, tilemaps in RAM), when not to (hot per-frame
  state, anything a lib C function walks through a plain pointer).
- `.claude/rules/bank0_budget.md` RAM twin: document the second band.
  Catalogue: B2 → shipped; B1 stays "subsumed by A6" as decided.

## 5. Risks

- **Silent cast-away.** `(u8 *)far_ptr` handed to a C helper that derefs
  bank-blind is the new silent failure. Mitigation: the qualifier-discard
  warning is on by default in cproc; the lib's public C helpers that walk
  user memory get a `__far`-tolerant signature or a documented "bank-0 only".
- **Corpus codegen drift.** Excluding tainted consumers from
  `temp_addr_only` must not change untainted code; the WRAM oracle catches
  it, and the compiler C→ASM checks pin the fused forms.
- **Boot time.** The 56 KB zero-fill is a one-off; measure it with luna and
  say the number in the crt0 comment.
- **wla-dx section semantics** for `BANK $7E SLOT 2` RAMSECTIONs are already
  exercised by the lib; no new linker surface.

## 6. Definition of done

Fixture green; corpus green on all three pillars plus the WRAM oracle;
breakout migrated with identical visuals; `KNOWN_LIMITATIONS.md`, `ABI.md`,
tutorial and catalogue updated; `FAR` documented in `<snes/types.h>`.

## 7. Decisions log

- 2026-09-05: plan written; option (A) recommended; awaiting the
  maintainer's call on (A) vs (B) and on the `__far` / `FAR` spelling.
