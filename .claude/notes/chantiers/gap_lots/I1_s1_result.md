# I1 S1: the cut label of a copied block (result)

Step S1 of `I1_inline_plan.md` (section 2, and point 2 of its Contest: the names of a unit `b.c`, `w.c`, `l.c`). Branch
`i1s1`, from `develop` `5bfaadca`, QBE at `3f469bf`. Pair: Opus writes and tests (no sub-agent, no penseur called on this
step: the change is a label repair and a guard, the plan was already contested).

## Facts read before the prediction (2026-11-14, today's `bin/`, nothing changed)

- **No label of the corpus ends in `.b`, `.w` or `.l` today** (either case): every definition line (`name:` and `@name:`)
  of the 78 corpus `.c.asm`, of the examples' assembly and of the four library object sets, scanned: 0.
- **A static function in a unit named `b.c`, `w.c` or `l.c`**, called from two sites (so not absorbed): its name is
  `g.b` (cproc decorates a file-scope static with the unit's stem), `jsl g.b` is read by WLA as `jsl g` with a size
  suffix. Built with today's tree:
  - alone: the link fails, `FIX_REFERENCES: Reference to an unknown label "g"` (b, w and l alike);
  - **with a global `g` in another unit: it links, silently, and calls the wrong function**: `r_b` read on luna
    `$AAB1` (two calls of the global `g`, which returns `$5555`, plus 7) where C says `$0025`.
- **The games' ROMs**: built by the gauge with this tree before any change (label `i1s1-avant`), both `cmp`-identical to
  `ref1114` (equivalence identical, 341 and 700 steps).

## Prediction (written and committed before the code)

1. Every `.c.asm` of the library (four object sets) and of the corpus is byte-identical before and after; every `.sfc`
   byte-identical (65/65 `cmp`), so `diff_corpus.py` 65/65 MATCH at offset 0. The repair touches only a label that ends
   in a dot and one letter of `bBwWlL`, and there is none (above).
2. The games' ROMs (gauge, label `i1s1-apres`) are `cmp`-identical to `i1s1-avant` and so to `ref1114`.
3. The new fixture (three levels of `static inline` copies, each cut falling right after `.b`, `.w`, `.l`) links and
   matches its host model in the main loop and in the NMI; under `QBE_INLINE_FAULT=label` the emitter guard stops the
   compile first, naming the label; with the guard also skipped (a second fault), the link fails with WLA's
   `Reference to an unknown label`.
4. A static in a unit `b.c` / `w.c` / `l.c` is refused by the compiler with a message that names the file's stem and
   says to rename it, instead of a link error or a silent wrong call. Nothing in the corpus or the games has such a unit
   (they build today, and the guard would have nothing to refuse in them, point 1).
5. The hunts (2000 + 2000 fresh seeds) pass: the compiler's output does not change on any label they produce (their
   units are not named `b`, `w` or `l`, and their copies do not nest three deep).

What would contradict it: one changed `.c.asm` (a cut the corpus already had: it would be named), a refused example
(a legitimate label of that shape: the guard would be wrong).

## Result (2026-11-14, measured after the prediction above was committed, `77b36b80`; code `5378e7e5`, qbe `f2e1f03`)

**Verdict: as predicted, on every point.** Nothing that assembled before changes; the new shapes are repaired or refused.

1. **Corpus and library byte-identical**: after `make clean && make`, the 1 049 files compared (65 `.sfc`, 78 + the
   examples' `.asm`, 345 `.asm` of the four library object sets) are `cmp`-identical to the build of `develop` taken
   before the change (`/tmp/i1s1_ref`). `diff_corpus.py --ref`: `DIFF CORPUS: 65/65 match, 0 diff, 0 missing (tolerance ±0)`.
2. **The games**: gauge label `i1s1-apres`, both ROMs `cmp`-identical to `i1s1-avant` and to `ref1114`; equivalence
   identical (341 and 700 steps); busy per step unchanged to the master cycle (368 101 and 609 299).
3. **The fixture** `testing/fixtures/compiler/inline_label_cut`: 24 repaired labels in `run()` (4 `.b_`, 16 `.w_`, 4
   `.l_`), values against the host model, NMI callback ran: all PASS. Faults, quoted:
   - `QBE_INLINE_FAULT=label`: `emit.c: dying: internal compiler error: block label
     inl5_f.main.12.inl3_pairer.main.10.inl1_bitsum.main.8.w of function run ends in .w, which the assembler reads as
     a size suffix (compiler/qbe/emit.c, wla_label_guard)`, make exit 2.
   - plus `QBE_LABEL_FAULT=noguard`: `main.c.wrap.asm:320: FIX_REFERENCES: Reference to an unknown label
     "run@inl5_f.main.12.inl3_pairer.main.10.inl1_bitsum.main.8"`, make exit 2.
   - The check `inl_cut` (stem `inl_cut`: the fixture's stem `inline_label_cut`, sixteen characters in every static's
     name, cannot put the cut on a suffix at any name length, measured over 22 x 22 lengths; `inl_cut` with names of 2
     and 4 does): PASS; under the first fault it fails to compile, under both it fails four lines (`missing .b_ / .w_ /
     .l_`, `unexpected \.[bwlBWL]:`).
4. **`b.c`, `w.c`, `l.c` (the Contest's point 2)**: today a link failure (`Reference to an unknown label "g"`), or, with a
   global `g` in another file, a **silent wrong call** (`r_b` = `$AAB1` on luna, C says `$0025`). After: refused at
   compile time, `qbe:b.c (IR line 37): function g.b ends in .b, which the assembler reads as a size suffix (it would
   call or read g): a file-scope static of a source named b.c is misread by the assembler; rename the file`; a static
   object gives `datum vv.B ...` (upper case checked by hand with a `B.c`). Pinned by `negative/b`, `negative/w`,
   `negative/l` (each turns `compiled` under `QBE_LABEL_FAULT=noguard`). A static of such a file that is absorbed
   everywhere (one site, or `static inline`) is never emitted and is not refused: it never reached the assembler. A
   static emitted but never referenced is refused although it would have linked: accepted, the message says what to do.
   So point 2's precondition for S4 ("a unit named b.c, w.c or l.c is refused for a candidate until the label rule of S1
   covers function names") is met: the guard covers function and object names.
5. **Hunts**: `difftest: 2000/2000 seeds clean, 48000 expressions in 4 shapes`, `difftest-stmt: 2000/2000 seeds clean,
   20000 programs` (seeds 56001-58000).

### Gates (this tree, branch `i1s1`)
- `make clean && make`: exit 0, the same 5 warning lines as the first build of this worktree (clang `-rdynamic`, CMake
  deprecation, three `-j forced`).
- `make test-compiler`: `Compiler checks: 155 passed, 0 failed (+0 compile-only; 0/0 unchecked ratchet; 34/34 refusals pinned)`.
- `make tests`: `ALL CHECKS PASSED (luna)` (the fixture's four lines PASS inside it; `176 passed, 0 failed` in its
  fixture block).
- `make lint`: exit 0. `make verify-toolchain`: `OK: toolchain pins match` (PINS.md names `f2e1f03`, 155 patches).
- `make test-sanitizers` was not run (the plan's common gates list it for a code step; the task's list does not).

### What is left
- qbe `f2e1f03` is on the submodule's local branch `i1s1`: it must reach the fork before `develop` can carry `5378e7e5`.
