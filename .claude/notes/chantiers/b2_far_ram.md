# Chantier B2 — C RAM beyond the 8 KB band (`__far` objects in bank $7E)

**Status:** DONE (2026-09-06) — Phases 0–4 on `wip/b2-far-ram`; ready to squash-merge into develop as a MINOR release. Follow-ups: the `cst`/`farram` flag pinning loads in QBE's optimiser (§10b); the HDMA A2A/NTRL hardware claim to verify against the corpus (§10f). Branch `wip/b2-far-ram`,
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


## 8. Phase 0 outcome (2026-09-05)

### 8a. The gate fixture — `devtools/compiler-tests/runtime/b2_far_ram/`

Objects placed by hand in `.RAMSECTION ".b2_far" BANK $7E SLOT 2`, accessed
from plain C. Wired into `make test-compiler` with the red cells as xfail
(an XPASS flags a stale entry once Phase 2 lands).

| Cell | Form | Today |
|---|---|---|
| `r_dir8/16/32` | symbol-direct load/store | red — `lda.w sym` / `sta.w sym`, DBR-relative (bank 0). *Read as green in the first Phase-0 run: store-to-load forwarding had replaced the load* |
| `r_fld8/16/32` | struct field via symbol | red — same `.w` forms |
| `r_hi` | bank byte of `&far_obj` | **green** (`$7E`) |
| `r_idx8/16` | `sym[idx]`, runtime idx | red — Kl add + `tax` + `lda.l/sta.l $0000,x` |
| `r_ptr8/16` | runtime pointer deref | red — same bank-blind forms |
| `r_ptr32` | runtime pointer, 32-bit | red — the **load** is already far (`[tcc__r9],y`), the **store** is not |
| `r_par8/parl8` | pointer through a param | red — RSlot path, bank-blind |
| `c0_*` | bank-0 controls | green |

Two cells passed by accident in the first run, both through store-to-load
forwarding: the param cell (the callee returned the value it had just
stored) and — found only once the far taint blocked forwarding in Phase 1 —
the direct-symbol cells (QBE had folded `x = far_u8` into the constant just
stored). Lesson for the fixture: a "green" cell is only meaningful when the
load is opaque; the taint makes every far access opaque, and the bank-0
controls stay forwardable on purpose (they test nothing about banks).

Corrected reading: **every** far access form is bank-blind today, including
symbol-direct (`lda.w sym`, emit.c 3896/3979; `sta.w sym`, 913/3694/3770/
3832). The linker-24-bit `lda.l sym` form exists only under the const taint
(3958/4002). Phase 2 starts there: it is the cheapest slice (`.w` → `.l`,
+1 cycle, +1 byte) and turns six cells green.

### 8b. Emit sites that assume bank 0 for a runtime address (`compiler/qbe/w65816/emit.c`)

| Site | Form | Fix in Phase 2 |
|---|---|---|
| 3715, 3752, 3785, 3850 | `sta.l $0000,x` (byte/word/long-low stores) | far-tainted → `sta [tcc__r9]` / `sta.l sym,x` |
| 3987, 4042 | `lda.l $0000,x` (word / byte loads) | already have a far twin under `volat & 2`; key both taints |
| 913, 3694, 3770, 3832 / 3896, 3979 | `sta.w sym` / `lda.w sym` symbol-direct (DBR-relative) | far-tainted → `sta.l sym` / `lda.l sym` |
| `Ostorel` high half | `sta.l $0002,x` | `,y` = 2 through the same base |
| 387 `temp_addr_only` + `is_high_dead_propagating` | drops the bank byte of address temps because every consumer discards it | exclude far-tainted consumers |
| 937 / 4857 indexed-long load fusion | `add($sym, idx)` + cst load → `lda.l sym,x` | mirror for stores; key on the far taint too |
| 981 `mark_addr_only_kl` premise | "bank-discarding opcodes" list | same exclusion |
| `compiler/qbe/emit.c` 159 / 293 / 333 | `.RAMSECTION ".bss.N" BANK 0 SLOT 1`, `.data_init.N` records `{addr16,size16}` | far kind → `BANK $7E SLOT 2`; record gains a bank byte |
| `templates/crt0.asm` 738 `CopyInitData` | `(tcc__r2),y` 16-bit destination, DBR = 0 | `[tcc__r2],y` with the record's bank |
| `templates/crt0.asm` 515–520 zero-fill | `$0000–$1FFF` only | add the `$7E:2000–$FFFF` fill (DMA to `$2180`) |

Lib audit: the hand-written ASM has **zero** DBR-relative indirect derefs
(`(dp),y`); every pointer walk is `[tcc__r0],y` (apu, lzss, snesmod) —
bank-honouring already. The C lib functions that take a *non-const* pointer
and would deref a far object bank-blind: `rectInit/SetPos/GetCenter/Contains`,
`collidePoint/Rect/RectEx`, `animTick/Restart`, `audioGetSampleInfo/VoiceState`,
`sramLoad/LoadOffset`, `dsp1Raster` (ASM, far-safe), `irqSet*`/`objInitFunctions`
(code pointers, n/a). Phase 3 list.

### 8c. The cost — `devtools/benchrom/b2_deref/` (`make -C … && python3 bench.py`)

Per access, loop overhead subtracted, one small-frame function per loop
(the first cut put every loop in `main`, blew the 255-byte frame and put
every local through `[tcc__fp],y`; those numbers were noise). Ratios matter,
absolutes are this compiler's stack-temp style.

| Access | near (today) | far (the #121 path a far object would get) | Δ |
|---|---|---|---|
| byte load, runtime pointer | 128 | 191 (`[tcc__r9]`) | **+50 %** |
| word load, runtime pointer | 132 | 237 | **+80 %** |
| byte load, `sym[idx]` | 125 | 105 (`lda.l sym,x`) | **−16 %** |
| byte store, runtime pointer | 100 | — (no far store path yet) | expect ≈ +60 |
| byte store, `sym[idx]` | 97 | — | expect ≈ near (fusion) |

Reading: the far pointer path pays for **restaging the 24-bit pointer into
`tcc__r9` at every access** (three DP stores), not for the `[dp]` addressing
itself (+1 cycle). Symbol-indexed far is free — better than near, because the
near path materialises the address in a Kl temp first. So:

- **Option (A) confirmed by data**: far-by-default (B) would tax every
  pointer walk +50–80 % today. (A) costs nothing on `sym[idx]` and on
  symbol-direct, which is what bulk buffers mostly are.
- **Phase 2 optimisation worth doing while in there**: hoist the `tcc__r9`
  staging when the pointer temp is unchanged between accesses (loop-invariant
  base, varying `y`), which turns the far pointer walk into `lda [tcc__r9],y`
  per access — near parity. Also the 32-bit far load already exists; make the
  far word load a single 16-bit `lda [tcc__r9]` (the byte path does
  `sep/lda/rep/and`).

## 9. Phase 1 outcome (2026-09-05) — the qualifier, end to end

Shipped in the submodules (cproc `feat/b2-far-qualifier`, qbe
`feat/b2-far-qualifier`) and `lib/include/snes/types.h` (`FAR`, empty under
the clang syntax lint since host compilers have no address spaces).

- **cproc**: `QUALFAR = 1<<5`, keyword `__far`, parsed with the other
  qualifiers, propagates through pointer types. Loads *and stores* through a
  far lvalue carry access-flag bit 4; objects get `section ".far"`.
  Diagnostics: static storage required, `__far const` refused, and a plain
  assignment dropping `__far` from a pointer is an error — cproc only checked
  qualifier discards in initialisers and arguments (`exprassign`), never in
  `p = q` (`mkassignexpr`), which is the one path that would have turned a
  far pointer into a bank-blind deref silently. Explicit casts still work.
- **QBE**: keyword `farram` (`far` collides with `sb` in the lexer's perfect
  hash — see `parse.c` K/M), bit 4 on the instruction; `section ".far"` →
  `.RAMSECTION ".far.N" BANK $7E SLOT 2`; initialised far data dies with a
  Phase-2 pointer.
- **cproc's qualifier model, learned the hard way**: `declarator()` stores
  the *base* qualifiers on each derived type — a pointer type's `qual` is its
  pointee's, an array type's `qual` is its element's — and returns the derived
  type's own quals as the declaration's `d->qual`. Two consequences:
  1. the object-qualifier rule is `d->qual | (array types' qual, walking
     nested arrays)`, never `type->qual` of a pointer;
  2. the pre-existing `.rodata` decision did consult `d->type->qual`, so a
     mutable `const T *p = …` global (or an array of pointers to const,
     `hero_metasprites[4]` in sprites/aseprite_pipeline) was sectioned into
     ROM. Masked for zero-initialised pointers (QBE's pure-bss branch wins),
     live for initialised ones — writes would have been silently lost. Fixed
     by the same `objqual()`; the aseprite WRAM baseline moved (16 bytes
     ROM→RAM+init record), visuals identical.
- **Fixture honesty**: with the far taint blocking store-to-load forwarding,
  the direct-symbol cells turned red — they had been green in Phase 0 only
  because QBE folded the load into the stored constant. All 13 far cells are
  now xfail; only the bank byte of `&far_obj` is green. `make test-compiler`
  green (5/5 + 13 xfail), corpus visual 85/85, WRAM 85/85 after the one
  justified update.
- **Clock skew bit again** (`clock_skew_incremental_builds.md`): the first
  `make compiler` after the edits rebuilt nothing — objects were dated 23
  minutes in the future. Every compiler rebuild in this chantier is
  `make -C compiler/{cproc,qbe} clean && make compiler`, and the corpus was
  rebuilt from clean before the oracle.

Phase 2 order, cheapest first: (1) symbol-direct `.w` → `.l` under the far
flag (six cells); (2) `sym[idx]` store fusion + load fusion keyed on far
(two cells); (3) runtime pointer stores via `[tcc__r9]` and the Kl store
high half, params fall out (five cells); (4) `temp_addr_only` exclusion;
(5) `tcc__r9` staging hoist; (6) initialised far data + CopyInitData bank +
crt0 zero-fill of `$7E:2000–$FFFF`.

## 10. Phase 2 outcome (2026-09-06) — the backend, end to end

All 25 fixture cells green (13 former xfail + 8 new: u32 `sym[idx]`,
struct fields through a far pointer, a far pointer walk, initialised far
data, the boot zero-fill). `KNOWN_FAIL` is empty; an XPASS/FAIL now names
the emit path. `devtools/compiler-tests/cases/far_ram_forms.checks` pins
the forms at compile time.

### 10a. Codegen (`compiler/qbe/w65816/emit.c`)

- One predicate, `is_far(i)` = `volat & 6` (const #121 **or** far B2):
  every #121 load path is now keyed on it; the store paths got their far
  twins. Symbol-direct: `lda.l/sta.l sym[+off]` (word, byte, Kl both
  halves). `stz` has no long form — far zero stores take `lda #0`.
  Byte-pair store fusion is far-aware (`emit_sta_conaddr(…, far)`).
- **Address decomposition** (`mark_far_decomp`, replaces the single-use
  `try_fuse_cst_index_load`): a far access whose address temp is a Kl
  `add` uses the add's operands —
  `add $sym, idx` → `emitload(idx); tax; lda.l/sta.l sym+off,x`;
  `add %p, N` → stage `p` in tcc__r9, `ldy #N`, `[tcc__r9],y`;
  `add %p, idx` → stage `p`, `emitload(idx); tay`, `[tcc__r9],y`
  (base = the Kl operand; when both are Kl the index is the one with
  `temp_high_zero`, else no decomposition). When every use of the add is
  such an access the add is not emitted (`far_decomp_all`) and its
  index operand's high half is dead (addr_only), its base's is a
  blocking use (the bank byte). Kl accesses reach the high half with
  `+2,x` / `ldy #N+2` / `iny iny`.
- **tcc__r9 cache** (`r9_ref`/`r9_valid`): `emit_cst_ptr_setup` skips
  the 3-store staging when the same ref is already there. Dropped at
  every block label and before any op that is not a load/store (Kl
  arithmetic and mul bodies use tcc__r9 as scratch; phi copies write
  temps). `p->a = …; p->b = …` stages once.
- `mark_addr_only_kl`: far loads AND stores are blocking uses (they read
  the bank byte through `[tcc__r9]`); the previous `Ostore* → always
  addr_use` rule was the pre-B2 "stores are bank-blind" premise.
- `temp_high_zero` now covers `extub`/`extuh` into Kl, not only `extuw`
  (cproc widens a u8/u16 index with those). Side effect on the whole
  corpus: the `Omul Kl pow2` / `Oshl Kl` narrow paths fire for u8/u16
  indices too (fewer cross-half `asl/rol`) — WRAM oracle drift, benign.
  `Oextub`/`Oextuh` also skip the high-half zero store when addr_only.
- Byte loads through an alloc'd local are excluded from the far pointer
  path (`getallocslot < 0`): a stack local's address temp has no valid
  bank half.

### 10b. cproc — attempted and REVERTED: `accessqual()`

`e->qual | e->type->qual` (A2) taints a load of a **pointer variable**
with its pointee's qualifiers (cproc keeps them on the pointer type): a
`T FAR *p` global reads as `lda.l p` (+1 cycle, correct) and, through a
struct pointer, takes the whole [tcc__r9] path; `const T *p` has done
the same since #121. A one-line fix (`accessqual()`: pointer-/array-typed
expressions contribute only `e->qual`) was tried and **reverted**: any
non-zero access flag also pins the load in QBE's optimiser (`gcm.c`,
`mem.c`, `load.c` test `i->volat` as a whole), so unpinning changed the
IR of 18 lib modules (allocs promoted away — `temp N: slot=-1 alloc=2`
— loads forwarded) and broke three examples at frame-equal comparison
(hdma/gradient_colors and hdma/hdma_helpers: CGRAM entry 1 zeroed;
basics/random: static screen). That is a latent QBE-optimiser-vs-backend
bug unmasked by wider optimisation, not a B2 matter. **Follow-up
chantier**: split the flag (bit 1 `cst` / bit 4 `farram` must not imply
"volatile" in the optimiser passes; only bit 0 should), then fix whatever
the promotion/forwarding exposes, with those three examples as the gate.
The taint asymmetry is harmless meanwhile — it only ever adds a far path.

### 10c. Data + runtime

- Init record is `{addr16, bank8, size16, bytes}` (5-byte header, no
  pad: `.data_init` is SEMIFREE in bank $00 and the ratchet sits at 12
  bytes on some examples). QBE emits `.db :sym`; `CopyInitData` reads
  the bank into `tcc__r2+2` and copies with `sta [tcc__r2],y`; the source
  stays `(tcc__r1),y`. End marker `.dw 0 / .db 0 / .dw 0`
  (`templates/data_init_{start,end}.asm`).
- Initialised far objects: `.RAMSECTION ".far.N" BANK $7E SLOT 2` + a
  record with bank $7E (the Phase-1 `die()` is gone).
- crt0 zero-fills `$7E:2000-$FFFF` at boot with one fixed-source DMA to
  `$2180` (`ZeroByte` in `.start`), before `InitHardware`, NMI still off.
  Measured with luna (`stats.total_mclk` across the `sta $420B`): **472 730
  master cycles = 22.0 ms**, 1.32 NTSC frames, once. NMI enable and the
  first game frame shift by that much; the corpus visual baselines that
  capture at a fixed instruction count moved on 8 animated examples for
  this reason alone (verified: same list minus those 8 with the fill
  compiled out).
- `symmap.py --check-ram-budget` prints a second line: far band top, free
  bytes, and how much of it is C `__far` (`.far.N` sections). No
  threshold.

### 10d. Cost (devtools/benchrom/b2_deref, cycles per access, loop overhead subtracted)

| Access | near | `__far` | Δ |
|---|---|---|---|
| byte load, runtime pointer walk `p[k]` | 116 | 121 (`[tcc__r9],y`) | +4 % |
| word load, runtime pointer walk | 132 | 137 | +3 % |
| byte load, `sym[k]` | 107 | 88 (`lda.l sym,x`) | −18 % |
| byte store, runtime pointer walk | 88 | 74 (`[tcc__r9],y`) | −17 % |
| byte store, `sym[k]` | 79 | 41 (`sta.l sym,x`) | −48 % |

Phase 0 had measured +50 / +80 % for the pointer walks: that was the
per-access re-staging of tcc__r9 (three DP stores) plus the Kl address
materialisation. The decomposition removes both; what remains is the
`[dp],y` form itself (+1 cycle) and the per-iteration staging at the
loop-body label. The `sym[idx]` forms beat near because near still
materialises the 24-bit address in a Kl temp before `tax`.

### 10e. Corpus validation

- Coverage 83 OK / 2 INPUT-DEP (unchanged). `make test-compiler` 18/18 +
  56 compile-only. Fixture 25/25.
- Visual regression: 18 labels moved. Verified by a **frame-indexed**
  comparison against a reference build of the previous commit
  (`scratchpad/frame_compare.py`: binary-search the step count where
  `scheduler.frame_count` reaches F, `--print-fbhash` both ROMs there,
  allow ±3 frames for the boot offset): 15 self-animating labels match
  at frames 200 and 400; `basics/random` differs only by its boot-time
  seed; the two `hdma/*` labels were glitched baselines (§10f). Two causes for the instruction-count
  drift: the 22 ms zero-fill (8 labels) and fewer instructions per frame
  from the `temp_high_zero` widening (`extub`/`extuh` sources, small
  scales) — the narrow `mul`/`shl` paths now fire on u8/u16 indices.
- WRAM oracle: all 85 drift (expected: crt0 + codegen), `--update`d.

### 10f. Found on the way: HDMA enabled mid-frame runs on stale A2A/NTRL (lib fix)

Three labels still differed at frame-equal comparison after the drift
analysis. Bisected over {zero-fill, extub addr-only skip, high-zero
widening}: `basics/random` and `hdma/gradient_colors` follow the
zero-fill only (random seeds from the frame counter at boot; gradient
below); `hdma/hdma_helpers` followed none of them. Both HDMA cases have
the same mechanism, which the hdma_helpers example already documents in
`stopCurrentEffect()`: the PPU copies A1T → A2A and loads the first table
entry only at the start of a frame, for channels enabled at that moment.
A channel enabled during active display (`setScreenOn(); hdmaSetup();
hdmaEnable()`) runs from the next HBlank with whatever A2A/NTRL hold —
zero at reset — so it reads a "table" at `$00:0000`, i.e. the `tcc__r*`
scratch bytes, and writes that residue to the destination register.
What the residue is depends on what ran before (CopyInitData leaves
different values with the 5-byte record) and on the exact scanline of the
enable (the 22 ms zero-fill moved it): the **reference baselines
themselves carried the glitch** — gradient_colors with CGRAM entry 2
zeroed (a black band in the logo frame), hdma_helpers with BG1HOFS stuck
at 2. luna showed both (`ppu.cgram`, `ppu.bgs[0].h_scroll`).

Fix at the lib layer (`lib/source/hdma.asm`, the three `hdmaSetup*`):
preset A2A = A1T and NTRL = 1 after programming the table, so the next
HBlank reloads the real first entry and a mid-frame enable starts the
table one line late and clean. Verified with luna: palette entries 1-15
equal the ROM palette, h_scroll 0, and both examples render identically
at frame-equal comparison between builds with and without the zero-fill
(deterministic against boot timing now). The HBlank procedure this relies
on (decrement, reload on zero) is anomie's; **to verify against the SNES
corpus** — cartouche was unreachable in this session (see
`.claude/rules/hardware_claims.md`, degrade path).

### 10g. Lessons

- **Rebuild the lib after ANY record-format change.** The first Phase-2
  run crashed every cell including the bank-0 controls: the compiler was
  rebuilt from clean but the lib's 108 `.c.asm` were not (clock skew
  again), so math/hdma/mode7's init records were still 4-byte headers
  and `CopyInitData` scattered their bytes over the stack (SP ended at
  `$2101`). `make clean && make` is the only safe sequence here.
- A fixture cell is only meaningful when the far access is opaque —
  still true; the new cells route through non-inlined functions.

## 11. Phase 3 outcome (2026-09-06) — lib + corpus

- **Type rule (cproc)**: a `const` target satisfies `__far`. Since #121
  every read through a const-qualified pointee is a far read (the backend
  takes the bank from the pointer), so `T FAR *` converts implicitly to
  `const T *` — the whole DMA/asset API (`dmaCopyVram(const u8 *…)`,
  `hdmaSetup(…, const void *)`, `bgInitTileSet`, …) accepts far buffers
  with no cast and no signature change. Dropping `__far` into a plain
  non-const pointer stays an error (initialiser, argument, assignment).
- **Far-tolerant signatures** where the ASM already honours the bank and
  the use is legitimately bulk: `sramLoad(u8 FAR *…)`,
  `sramLoadOffset(u8 FAR *…)` (`mvn $70,$7E` — bank $7E, whose first 8 KB
  mirror bank 0, so both placements were always right),
  `dsp1Raster(u8 FAR *ab, u8 FAR *cd, …)` (reads both bank bytes). Near
  callers are unchanged (near → far is implicit). ABI lint parses `u8
  FAR *` as a 4-byte slot; 290 signatures, no mismatch.
- **Kept bank-0-only, by design**: the C helpers that walk small hot state
  through a plain pointer — `rectInit/SetPos/GetCenter/Contains`,
  `collidePoint/Rect/RectEx`, `animTick/Restart`,
  `audioGetSampleInfo/VoiceState`. Their objects belong in the 8 KB band
  (per-frame state), and the compiler refuses a far pointer there without
  an explicit cast, so there is no silent path. Documented in Phase 4.
- **games/breakout**: `.game_buffers` (blockmap, backmap, pal, blocks —
  4708 B) → `.RAMSECTION BANK $7E SLOT 2`, `extern FAR …` on the C side,
  the three local helpers take `FAR` pointers, the DMA casts become
  `(const u8 *)`. C RAM band **1436 → 6880 bytes free** (the ORGA $0800
  hole is gone too). Codegen: `blocks[b]` = `lda.l blocks,x`,
  `blockmap[i] = v` = `sta.l blockmap,x`, six `[tcc__r9]` derefs in the
  copy/text helpers. fbhash identical.
- **mode7/dsp1_ground**: the double-buffered M7 tables (2 × 1030 B) →
  `static FAR u8 tab_ab[2][…]`; `hdmaSetup` takes the far pointer, the
  DSP-1 raster writes through `[tcc__r9]`. C RAM **1744 → 3937 bytes
  free**. fbhash identical.

## 7. Decisions log

- 2026-09-05: plan written; option (A) recommended; awaiting the
  maintainer's call on (A) vs (B) and on the `__far` / `FAR` spelling.
- 2026-09-05: Phase 0 done (§8). (A) now backed by the bench: far runtime
  pointer +50/+80 %, far `sym[idx]` −16 %.
- 2026-09-05: maintainer chose (A), spelling `__far` / `FAR`. Phase 1 done (§9).
- 2026-09-06: Phase 2 codegen done (§10); the single-use index fusion is
  subsumed by the general decomposition; init record gains a bank byte.
  cproc `accessqual()` tried and reverted (§10b) — follow-up chantier.
- 2026-09-06: Phase 3 done (§11): `const` satisfies `__far`; breakout
  1436 → 6880 bytes free in bank 0.
- 2026-09-06: Phase 4 done — `docs/tutorials/far_ram.md`, KNOWN_LIMITATIONS
  entry 🔴 → 🟡 with the `FAR` escape, `compiler/ABI.md` address-space
  section, `bank0_budget.md` RAM twin, catalogue B2 → 🟢 shipped,
  `snes/types.h` doc. Chantier complete.
