# C1 — audit des modules 100 % ASM (migration mesurée vers le C)

Status: COMPLETE — all 5 modules decided (2026-07-19/20). Outcomes:
audio.asm deleted (audio v2), mode7 MIGRATED to C, map KEEP,
sprite_dynamic KEEP (hot core; cold paths already C), snesmod KEEP
(provenance). Policy doc: lib/ARCHITECTURE.md. Originally opened Owner-validated as
a structural priority ("précisément l'esprit de ce projet").
Catalogue entry: `.claude/STRUCTURAL_DEFECTS.md` C1. Precedent:
`audio.asm` (was the worst of five) retired by the audio v2 chantier
— not migrated but rebuilt in C on a measured foundation.

## The rule of the game (from the catalogue, non-negotiable)

Every keep/migrate decision is **benchmark-backed**. Migrate when the
C port regresses ≤ +10 % on the module's hot paths (maintainability
wins); keep ASM when it doesn't, extracting cold paths to C and
documenting the hot-path boundary + internal invariants. No decision
by intuition — the QBE backend has had 28 perf patches since the ASM
was written; the gap must be re-measured, both directions.

## Inventory (2026-07-19)

| Module | LOC | Public fns | Linked by | First read |
|---|---|---|---|---|
| `mode7.asm` | 481 | 10 (`mode7.h`) | 3 examples | smallest, well-bounded — **first target** |
| `sprite_dynamic.asm` | 1232 | via `sprite.h` (dispatch + helpers split) | 5 examples | hot NMI-adjacent paths (VRAM queue) |
| `map.asm` | 1240 | 7 | 4 examples | collision getters already partly C-fronted (#103) |
| `snesmod.asm` | 1201 | 16 | snesmod_* via dedicated wiring | mukunda's driver port — expected verdict: KEEP + document (upstream parity > rewrite) |

Order: **mode7 → sprite_dynamic → map → snesmod**. Each module = one
wip branch, one squash-merge, decision recorded here.

## Phase 0 deliverable: the instrument (benchrom)

A cycle-benchmark fixture, libtest-style: `devtools/benchrom/` — a
ROM that runs each measured function in a tight N-iteration loop
bracketed by frame counters, storing per-function frame counts in
WRAM globals; a python runner (luna `state --peek`, symbol names)
converts to ~cycles/call and prints a table. Relative precision is
what matters (same harness measures the ASM original and the C port);
absolute cycles are informative only. The instrument lands BEFORE any
migration and each module's baseline numbers are committed to this
note — the same discipline as #120's provenance: no "it felt fast".

Measurement points per module = its hot paths as named by the
catalogue + the functions its examples actually call (grep-derived,
not guessed).

## Baselines — mode7.asm (ASM original, 2026-07-19)

Instrument: `devtools/benchrom` (20 000 iterations/fn, frame-count
brackets, empty-loop calibration 53.6 cyc/iter subtracted; ~59 561
CPU cycles/frame NTSC). `python3 devtools/benchrom/bench.py`.

| Function | frames | ~cycles/call |
|---|---|---|
| mode7SetAngle | 203 | **551** |
| mode7SetScale | 47 | 86 |
| mode7SetCenter | 53 | 104 |
| mode7SetMatrix | 73 | 164 |
| mode7Transform | 275 | **765** |

Per-frame budget context: an example calling SetAngle every frame
spends 551/59 561 ≈ 0.9 % of the frame in it. The +10 % rule
therefore allows the C port ≈ 606 cyc for SetAngle, ≈ 842 for
Transform. The cheap register-stuffers (SetScale/Center/Matrix) are
prime C candidates; the trig paths (SetAngle/Transform) are where
the LUT indexing quality of the codegen will decide.

## mode7 — C-port experiment, interim state (wip/c1-mode7, 2026-07-19)

Faithful C port written (`mode7.c`: same LUT bytes, PPU-multiplier
trick via volatile, math.c non-static-const precedent for the table).
Three measured iterations:

| Function | ASM | C helpers | C macros (best) | C u8-temps | Δ best vs ASM |
|---|---|---|---|---|---|
| mode7SetAngle | 551 | 2639 | **1144** | 1230 | +108 % |
| mode7SetScale | 86 | 74 | **74** | 74 | **−14 %** ✓ |
| mode7SetCenter | 104 | 685 | **146** | 211 | +40 % |
| mode7SetMatrix | 164 | 1310 | **259** | 366 | +58 % |
| mode7Transform | 765 | 3008 | **1513** | 1599 | +98 % |

Lessons already worth their price:
1. **Helper functions are poison at this scale**: ~340 cyc/call
   overhead (post-A6 pointer args + frame) — macros only.
2. Pure-RAM setters are FASTER in C (SetScale −14 %): frameless leaf
   codegen beats the ASM's php/plp bracket.
3. The remaining gap is **sep/rep churn around MMIO byte stores**:
   the emitter re-enters 16-bit mode after every 8-bit store because
   the next value load is 16-bit; the ASM stays 8-bit for a whole
   function. Pre-extracting u8 temps makes it WORSE (stack traffic).

### Final state after the byte-pair store fusion (emitter peephole)

The emitter lever WAS taken (qbe ea2372a: storeb+shr8+storeb fused
into lda16/sep/sta/xba/sta — fires corpus-wide on every 16-bit-to-
8-bit-MMIO pair) plus two C-side lessons (cache globals in locals so
the fusion sees one temp; read MPYM/MPYH as ONE volatile u16 like the
ASM's lda.l \$2135; LUT as initialized RAM static — as ROM const it
spilled to bank \$01 on mode7_perspective and the symmap guard caught
the garbage-read).

| Function | ASM | C final | delta | +10 % rule |
|---|---|---|---|---|
| mode7SetScale | 86 | 74 | -14 % | PASS (faster) |
| mode7SetCenter | 104 | 110 | +6 % | PASS |
| mode7SetMatrix | 164 | 185 | +13 % | FAIL by 21 cyc |
| mode7SetAngle | 551 | 765 | +39 % | FAIL (+214 cyc) |
| mode7Transform | 765 | 1135 | +48 % | FAIL (+370 cyc) |

Semantics: 70/70 fbhash bit-identical, probes 17/17, libtest 31/31 —
both the fusion (whole corpus) and the C port (3 mode7 examples)
proven behaviour-preserving. Absolute context: SetAngle is the only
per-frame path in real examples; +214 cyc = 0.36 % of a frame.

**VERDICT (owner, 2026-07-19): MIGRATED — documented rule exception.**
The +10 % rule is waived for the trig paths on the absolute-cost
argument above (0.36 % of frame on the sole per-frame caller), traded
for deleting 481 lines of ASM and the corpus-wide fusion peephole the
experiment forced into existence. Precedent for future modules: the
rule stays, exceptions must be argued in absolute cycles ON the
real per-frame paths, and every migration must pay its way in emitter
improvements.

Residual gap on the trig paths: LUT indexing + s8 sign extensions +
the Transform->Rotate->SetAngle call chain. Closing it needs more
emitter work (sign-extension patterns, call-frame cost).

Original lever list (for the record):
- **(a) Emitter improvement**: batch/avoid the rep between an 8-bit
  store and a following 16-bit load whose only consumer is another
  8-bit store (or a mode-aware store-pair peephole). Benefits the
  whole corpus (every MMIO-heavy module), in the tradition of the 28
  QBE perf patches — the C1 audit fixing the COMPILER instead of
  keeping ASM is the best possible outcome.
- (b) Per-function split (C for cold, ASM for SetAngle/Transform) —
  two sources per module, the C2-duplication smell; last resort.
- (c) Keep mode7.asm as-is, document invariants — the fallback the
  rule allows.

Absolute context for (a): SetAngle's +593 cyc ≈ +1 % of a frame for
the one per-frame caller. The +10 % per-function rule stands, but the
emitter lever could close most of it corpus-wide.

NOT validated yet: the 3 mode7 examples (visual + WRAM) against the C
port — required before any merge regardless of the perf verdict.

## Baselines — map.asm (ASM original, 2026-07-19)

Same instrument, real mapscroll data (bank-2 pinned blobs as in
libtest), 20 000 iterations. mapVblank measured OUTSIDE VBlank (PPU
ignores the writes; the CPU work measured is identical). The camera
sweep `i & 511` pans 1 px/call, so column streaming fires on ~1/8 of
iterations — composite numbers, same composite for the C port.

| Point | ~cycles/call |
|---|---|
| mapGetMetaTile | 208 |
| mapGetMetaTilesProp | 223 |
| mapUpdateCamera | 217 |
| camera+mapUpdate | 1114 (mapUpdate alone ≈ 897) |
| camera+update+mapVblank | 1325 (mapVblank alone ≈ 211) |

Migration obstacle flagged up front: map.asm's bulk state (metatile
defs, properties, row LUT, tilemap buffers) lives in a BANK $7E
RAMSECTION above the C-visible WRAM mirror (B2: C RAM must be below
$2000). A C port would need far-pointer access for every buffer
touch, or the buffers stay in ASM-owned sections with C accessors —
the verdict may legitimately differ from mode7's.

**VERDICT (2026-07-19): KEEP AS ASM — probe-backed.** A C model of
the LIGHTEST far path (mapGetMetaTile: 2 far reads through cached
4-byte pointers, scalars in C statics — the best a full port could
architecturally do) measured **277 cyc vs 208 ASM (+33 %)**, already
past the rule before the buffer loops (dozens of $7E accesses per
streamed column on the VBlank-adjacent path — far access ~13 cyc vs
DB-direct 5-6). The ~10 KB bank-$7E state cannot move below $2000
(RAM band would blow on tight examples). Kept-obligations delivered:
invariants documented in the module header (DB discipline, the #103
open-bus class, NMI budget, section split); ABI-lint coverage
confirmed (no skip-file marker). Future emitter idea recorded, NOT
scheduled: a "pinned data bank" region concept could close the far
gap — well beyond this audit.

(Probe history: the original 277-cyc measurement was actually reading
bank $00 garbage — pointer derefs were bank-0-hardcoded pre-#121
(that's also why its correctness pin failed). Post-#121 the far reads
are REAL and cost 399 cyc (+92 % vs ASM 208) — the KEEP verdict is
reinforced, not weakened, by the honest number. The sym-direct
`lda.l sym,x` fusion doesn't apply to the probe's cached-pointer
model, which matches a full port's architecture.)

## Acceptance criteria (per module, from the catalogue)

1. A written keep/migrate decision in this note with the benchmark
   table (ASM vs C, cycles/call, regression %).
2. Migrated: suite + probes green, WRAM/fbhash rebaselines documented,
   ROM-sha churn justified, ≤ +10 % on hot paths.
3. Kept: cold paths extracted to C where sensible; an internal-
   invariants doc (struct offsets, register state, .ACCU discipline)
   in the module header; ABI-lint coverage confirmed.
4. End of chantier: `lib/ARCHITECTURE.md` (or PHILOSOPHY.md section)
   states the C/ASM split policy going forward.

## Risks

- **NMI-adjacent paths** (sprite_dynamic's queue flush): regressions
  here eat VBlank budget — the +10 % rule applies to the VBlank-
  critical section specifically, not the average.
- **snesmod**: upstream parity is worth more than C purity; the audit
  may legitimately conclude "keep everything, document". That is a
  SUCCESS outcome, not a failure of the chantier.
- **Baseline churn**: every migration shifts return addresses → WRAM
  stream rebaselines. Clean-build discipline (clock-skew machine!)
  and per-phase ROM-invariance checks as in audio v2.


## Baselines + verdict — sprite_dynamic.asm (2026-07-20)

Instrument extended (16x16 engine inited as the dynamic_sprite
example; draws MUST pair with EndFrame — the slot allocator resets
there, unpaired draws hang the engine, discovered the hard way):

| Point | ~cycles |
|---|---|
| draw+EndFrame (steady pair) | 968 |
| refresh+draw+EndFrame+flush (full lifecycle) | 2076 |
| NmiFlush idle floor (per-frame NMI cost) | 283 |
| **C model** of steady draw + ASM EndFrame | 1555 → **1486** after a codegen-friendly rewrite |

**VERDICT: KEEP AS ASM.** The isolated C draw is ~+50 % over the ASM
one even in its best shape: the ASM pins X (OAM offset), Y (oambuffer
offset) and DB ($7E mirror) across the whole function; compiled C
re-derives every array index per statement (240-byte frame, heavy
spill traffic). That is a cross-statement register-allocation gap —
not reachable by peepholes — on the module where cost multiplies by
sprite count per frame and partly sits inside the NMI VBlank budget
(strict rule, no absolute-cost exceptions). The module is already the
target hybrid (dispatch/helpers/meta in C). Invariants documented in
the header.

## Verdict — snesmod.asm (2026-07-20)

**KEEP — provenance, not perf (no benchmark needed).** mukunda's
driver via KungFuFurby's WLA-DX port; its value is parity with the
upstream SNESMOD ecosystem (the sm_spc.asm SPC700 blob and this 65816
half are a matched pair; tracker tooling targets their exact
behaviour). A C rewrite = a fork for zero functional gain. The
OpenSNES-native C audio path is audio v2. Rationale in the header.

## AUDIT CLOSED — 2026-07-20

Five modules, five decisions, every one either benchmark-backed or
provenance-argued. Final split + the going-forward policy:
lib/ARCHITECTURE.md. Emitter gaps recorded for the future: pinned
data bank (map), cross-statement index register allocation
(sprite_dynamic). benchrom keeps all baselines for re-runs.
