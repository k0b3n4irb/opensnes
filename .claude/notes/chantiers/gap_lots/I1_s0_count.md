# I1 S0: the count before any code (inline the small functions of the same file into their callers)

The second idea drawn from the comparison with the other 65816 compiler (the owner's go of 2026-11-14): on the gapbench
routine `call` (calls to a small function of the same file) it runs at 0.41 of ours, because it inlines. A count: no
compiler change, no ROM built differently. The names are neutral (jeu 1, jeu 2, the games).

## What inlining does today (source read, `compiler/qbe/inline.c`, `compiler/qbe/main.c`, branch `develop` `812a168b`)

Two inliners, both in QBE, both on one unit (the `.c` file) at a time, run between parsing and the rest of the pipeline
(`emit_collected` in `main.c`: count the direct call sites and the address-taken names over the whole unit, then
`inline_check` on every function, then the address-taken scan again, then emit, skipping what was absorbed):

1. **The small inliner** (`inline_record` / `save_body` / `splice_at`, 2026-05): a function **written `inline`** in the
   source (`fn->lnk.inline_hint`, set by cproc when the C carried the keyword, `cproc/qbe.c:1782`) whose body is
   straight-line (no conditional jump, no phi), calls nothing, has no alloc, has no section of its own, and is at most
   **16 IR instructions** (`INLINE_MAX_INSTR`, `CC_INLINE_MAX_INSTR=N`) is pasted at every direct call. Exported or not:
   it serves the library's C99 `inline` helpers of the headers (`getFrameCount`, `fix32Abs`, `gsuIsPresent`, ... 9 in
   `lib/include/snes`; one `static inline` in `sprite.h`). Its standalone body is not emitted when every call was taken
   and no address was used (`inline_fully_consumed`).
2. **The whole-function inliner** (`auto_inline_ok` / `splice_fn`, 2026-10-10): any control flow, calls inside allowed.
   Taken when the callee is **not exported** (`static`), its **address is never taken** (`addrtaken`: a data reference or
   any code reference that is not a direct call's target), it is not the caller itself and not on the current chain (a
   cycle, `busy`: declined), has no section of its own (`__ramcode`), no variadic or struct parameter or return, no alloc
   left (a local whose address escapes), and caller + callee stay under 1 800 temporaries; **and** either it has
   **exactly one direct call in the unit** (`rec->sites == 1`, counted before any inlining) whatever its size, or it is
   written **`static inline`** and is at most **160 IR instructions** (`INLINE_MAX_BIG`, `CC_INLINE_MAX_BIG=N`), then
   copied at every site. The unit is walked bottom-up (`inline_check(rec->fn)` before the splice: the body cloned is
   final, so nesting is to any depth). The leaf-size refusal (`CC_INLINE_LEAF_MAX`) exists and is off by default since
   2026-10-10.

So what stops a same-file call from being inlined today: **an exported callee** (any function without `static`, even
called once); **its address taken**; **two or more call sites** without the `inline` keyword (the decisive one: no size
criterion at all exists for a plain `static`); recursion or a cycle; an alloc kept (address-taken local); variadic or
struct parameters; a `__ramcode` section; the temporary limit. A plain `static` with several sites is never inlined,
however small: there is no size heuristic. No cross-unit inlining (each `.c` is compiled alone; a call into another
file, the library included, is never inlined, except the header `inline` helpers which are in the unit).

## Prediction, written 2026-11-14 before the count (committed alone, before any census or script exists)

Nothing below is measured. Inputs already published: L4 M1 (`L4_frames_and_calls.md`, part 2) priced `jsl` -> `jsr` at
14 mclk per executed same-file call and found 0.30 / 0.31 % of busy, i.e. about **6 250 (jeu 1) and 7 900 (jeu 2)**
executed same-file `jsl` on the gauge's 4 x 40 frames (A1 S0 recounted 6 248 / 7 913 on `ref1114`); busy on those
traces 27 866 030 and 34 243 412 mclk. Every one of those calls is, by construction, to a callee NOT inlined today (it
is still a `jsl` in the text). Median measured costs (A1 S0, the same traces): `pha` 28 mclk, `plx` 34, `lda n,s` 34.
Both games are FastROM: `jsl` is 8 cycles (4 fetches at 6, 1 internal, 3 stack writes at 8) = **54 mclk**, `rtl` 6
cycles (1 fetch, 2 internal, 3 pulls at 8) = **44**.

- **Overhead removed per executed call, lower bound (no constant propagation)**: `jsl` + `rtl` 98; one argument: its push
  28, its cleanup 34 (one `plx`), its read `lda n,s` 34 in the callee, against a replacement of at most one direct-page
  access (28): net about 68; a framed callee adds the frame carve and its undo (`tsc / sec / sbc / tcs` and back, about
  108), which the inlined body does not pay at each call (the caller's frame grows once). Hypothesis on the mix (A1 S0:
  the executed calls are mostly one-argument calls to small helpers, a minority framed): **about 180 mclk per call**,
  range 140 to 220.
- **Share eligible** (hypothesis H1): callee not exported, address never taken, not recursive. A1 S0 found its own
  eligible set (static, Kw first) already holding 89 % and 79 % of the executed same-file calls; no-parameter statics
  add to that, exported callees called from their own file are rare (A1-W: 0 and 840 calls). I take **0.85**, range
  0.75 to 0.95.
- **Figure**: 6 250 x 0.85 x 180 / 27.87 M = **3.4 % (jeu 1)**; 7 900 x 0.85 x 180 / 34.24 M = **3.5 % (jeu 2)**; with
  the ranges: **2.4 to 4.7 % (jeu 1), 2.3 to 4.6 % (jeu 2)**. Upper bound with constant propagation ("hypothèse", not
  in the rule): a constant argument then lets the callee's tests on it fold; I do not price it before the count.
- **ROM**: duplicating a callee of B bytes at S sites costs about (S - 1) x B minus the call sequences removed. The
  games' ROMs have whole free banks (jeu 1: about 25 empty banks of 32 KB out of 64; jeu 2: about 46 of 128, from
  `opensnes-rom budget` on the `ref1114` ROMs); code is `SUPERFREE` and may go to any bank. Prediction: tens of KB for
  everything eligible, **it fits** on both games; only bank $00 (60 bytes free) is full, which no longer fails a link.
- **The rule** (written here, before the count): **at least 1.5 % of one game's busy time, in the lower bound (no
  constant propagation), with an ROM cost that fits the game's free space; otherwise the lot stops.**
  **Prediction: above 1.5 % on both games, the lot goes on to its next step.** What would contradict it: an eligible
  share under 0.5 (most hot callees exported or address-taken), or a saving under 100 mclk per call (most calls
  argument-free to frameless leaves, then only `jsl` + `rtl`).
- **What this kind of count cannot see, said before** (LF, M5, L4 S3 were each wrong at execution, in both directions):
  the count prices today's text minus the call; it does not see what the compiler does with the bigger caller. A leaf
  callee poured into a caller that calls loses its `tcc__lf` frame for the caller's: its temps that cross a call go to
  the stack (one cycle more per access); the 2026-10-10 measurement of the whole-function inliner found a game's logic
  2 % SLOWER with every leaf absorbed under the rules of that morning, then 0.7 % faster with the later rules. The
  peephole's straight lines change shape, the X/Y colour and the loop rules may gain or lose a counter. So the count is
  an upper bound of the call-overhead part and blind to the register part; the step after S0 must recompile.
- **Coherence check announced**: the census's executed `jsl` to a C function of the same unit must equal A1 S0's
  6 248 / 7 913 on the same `ref1114` traces (and M1's rule on the trace).

## Result of the count (2026-11-14, measured after the prediction above was committed, `180bc1bf`)

**Verdict of the rule: the lot goes on.** In the lower bound (no constant propagation), inlining every eligible callee of
at most 256 bytes, nested as today's splicer nests, reaches **2.55 % of busy time (jeu 1) and 2.35 % (jeu 2)** for
**+68 KB and +160 KB of ROM**, against 885 KB and 1 581 KB free: above 1.5 % on both games, and in every gauge window
(jeu 1 2.26 to 3.83, jeu 2 1.62 to 2.75). Inlining *everything* eligible would give 4.89 / 4.54 % but does **not** fit
(nested: +7.5 MB and +2.2 MB): a compiler needs a size criterion, and 256 bytes is one that holds. Nothing here is
recompiled text; the check that is comes next, with its own prediction written before it runs.

### Build, trace, method

- The same inputs as A1 S0 (`A1_s0_count.md`): the gauge's frozen reference `ref1114`, both games' `.c.asm` with the side
  table, ROM, `.sym`; the IR kept by `CC65816_KEEP_IR` on builds whose ROMs are `cmp`-identical; the library's LoROM
  `.c.asm` and IR; the same luna traces (4 x 40 frames a game; busy 27 866 030 and 34 243 412 mclk).
- `devtools/sframe/i1_count.py` (generic, `make count-i1`; it imports `a1_count.py`: table, text, IR, ROM placement,
  trace, prices). Sites: every direct `jsl` of a C function to a C function of the same unit in the text (a `jml`, a
  tail call, apart). Every instruction is placed at its ROM address (390 / 389 functions, 0 mismatch, as in A1 S0) and
  weighted by its executions; each removed instruction is priced at the median measured cost of its shape in the same
  trace (`jsl` 54 mclk, `rtl` 42, `pha` 28, `pea` 34, `plx` 34, `tsc` / `tcs` / `clc` / `sec` 12, `adc #` / `sbc #` 18).
- **Lower bound, per executed site**: the `jsl`; the argument pushes, except that a pushed value with no home the copy
  could read (anything but a constant `pea`, a `pei`, or `lda n,s` / `lda tcc__lf+N` / `lda #k` just before the `pha`)
  is charged a direct-page store; the cleanup (`plx`, or `[tax] tsc clc adc tcs [txa]`); and, per entry of the callee
  (its executions divided by its entries), the frame carve at its head, the undo before each `rtl`, the `rtl`, and a
  `jml` inside it becoming a `jsl` in the copy (+30 mclk). The callee's reads of its parameters are kept at today's cost
  (read as an access to the argument's home). **Upper bound (hypothesis)**: also every parameter read free and every
  argument load free (the value already where it is used, a constant folded into its use); no branch folding counted.
- **ROM**: bytes from the ROM placement (the text estimate is within 0.2 % on the placed functions: 335 / 310 exact,
  the others count an 8-bit immediate as 3 bytes); a copy is the callee without its head and tail pieces and its `rtl`,
  plus a 3-byte jump per extra return; minus each site's call sequence; minus the standalone when nothing else calls it.
  **Nested** (bottom-up, a copy holds the copies of its own inlined callees: today's splicer) and **one level** (a site
  inside an inlined callee stays a call, and that callee's standalone stays for it).

### The coherence check (mission point 6)

Executed same-file `jsl`, summed over the text's sites: **6 248 (jeu 1), 7 913 (jeu 2)**, the same figures as A1 S0 on
the same trace; M1's rule replayed on the trace 6 247 and 7 909 (the calls followed by the NMI, as A1 S0 listed); M1's
own figure on its build `l2`: 6 248 and 8 011 (jeu 2's compiler has changed since). Totals agree.

### (1) Census: same-file calls not inlined today

| | corpus (65 examples) | library alone | jeu 1 (project + library) | jeu 2 |
|---|--:|--:|--:|--:|
| same-file `jsl` sites (text) / `jml` | 438 / 4 | 151 / 7 | 928 / 18 | 1 455 / 103 |
| callees called | 139 | 49 | 276 | 349 |
| of which **eligible** (not exported, address never taken, not recursive) | **122** | **21** | **228** | **305** |
| refused: exported / address taken / recursive | 17 / 0 / 0 | 28 / 0 / 0 | 46 / 1 / 1 | 42 / 0 / 2 |
| eligible `jsl` sites | 418 | 99 | 847 | 1 309 |
| executed same-file calls / of which to an eligible callee | | | 6 248 / **6 089** (97.5 %) | 7 913 / **7 010** (88.6 %) |
| eligible callees executed in the window | | | 50 | 57 |
| eligible callee size, bytes: min / quartiles / max | | 23 / 49 / 79 / 128 / 358 | 4 / 61 / 127 / 301 / 4 364 | 11 / 78 / 179 / 335 / 5 696 |

None of the eligible callees keeps an alloc, none is variadic, no unit lacks its IR. Sites per eligible callee (jeu 1 /
jeu 2): 2 sites 93 / 85, 3 sites 57 / 45, 4 to 8 sites 50 / 45, 9 or more 18 / 29 (jeu 2's hottest callee has 72), and
**one site 10 / 101**: callees the whole-function inliner should already take (static, one call in the cproc IR too)
and declines. Read with `CC_TRACE_INLINE=1` on jeu 2's object unit: its two big callers absorb some and decline the rest
(one absorbs 17 and declines 18, the other 10 and 90); hypothesis, the trace does not print the reason: the
1 800-temporary room (`AUTO_INLINE_TMP_ROOM`) of a caller that grows with each absorption. Those one-site callees hold
**0.81 % (jeu 1) and 0.75 % (jeu 2)** of busy time in the lower bound, inside the figures above (jeu 1's fourth callee
is one). 13 / 96 sites whose push sequence is not straight (a label or a nested call among the pushes) are priced with
the `jsl` and the callee part only.

### (2) Gain per site (lower bound) and (3) in % of busy time

| | jeu 1 | jeu 2 |
|---|--:|--:|
| mean saving per executed eligible call (mclk) | 223.9 | 221.9 |
| caller side (`jsl`, pushes, cleanup), % of busy | 3.050 | 2.718 |
| callee side (frame carve and undo, `rtl`), % of busy | 1.842 | 1.823 |
| **every eligible callee, nested (does not fit in ROM)** | **4.892** | **4.542** |
| upper bound, hypothesis (parameter reads and argument loads free) | 7.604 | 7.216 |
| tail calls (`jml`) added | +0.003 | +0.009 |
| executed calls by arity 0 / 1 / 2 / 3+ | 526 / 3 781 / 849 / 933 | 619 / 4 142 / 1 676 / 573 |
| framed callees: calls, % of busy | 2 457, 2.64 | 3 139, 2.47 |

By callee size (a compiler's criterion; % of busy, lower bound; ROM in bytes):

| callees up to | jeu 1 nested | ROM | one level | ROM | jeu 2 nested | ROM | one level | ROM |
|---|--:|--:|--:|--:|--:|--:|--:|--:|
| 32 B | 0.01 | +1 258 | 0.01 | +1 254 | 0.10 | +718 | 0.10 | +718 |
| 64 B | 0.27 | +4 511 | 0.27 | +4 312 | 0.74 | +19 002 | 0.74 | +16 538 |
| 128 B | 0.74 | +24 540 | 0.71 | +15 332 | 1.28 | +45 655 | 1.26 | +27 718 |
| **256 B** | **2.55** | **+68 163** | 2.48 | +35 781 | **2.35** | **+160 370** | 1.90 | +54 033 |
| 512 B | 3.87 | +292 621 | 3.29 | +49 987 | 3.99 | +259 179 | 2.39 | +58 813 |
| 1 024 B | 4.08 | +355 608 | 3.46 | +56 170 | 4.31 | +428 406 | 2.70 | +62 869 |
| all | 4.89 | +7 505 187 | 1.54 | +39 690 | 4.54 | +2 203 724 | 1.26 | +81 291 |

"One level" with every callee falls to 1.5 / 1.3 because the big callees then hold the calls of the small ones as calls:
one level only makes sense under a size limit, where it stays close to nested. The gain is concentrated: in the
profile's order (gain per byte, which no compiler knows) three callees reach 1.64 % for +1.5 KB in jeu 1, six reach
1.80 % for +8.8 KB in jeu 2. Per gauge window, callees up to 256 B nested: jeu 1 3.83 / 2.41 / 2.26 / 2.29, jeu 2 2.74 /
2.75 / 2.52 / 1.62; every eligible callee: jeu 1 4.46 to 5.08, jeu 2 2.39 to 5.75.

### (4) Eligibility, and the table it reuses

Safe by construction for a copy that replaces the function: **not exported** (`; sframe fn ... export=0`: nothing
outside the unit can call it), **address never taken** (`addrtaken=0`, and not in the unit's `; sframe addr` lines: no
indirect call), **not recursive** (not in a cycle of the unit's direct calls, itself included: Tarjan on the text's
`jsl` / `jml` edges, the edges L4 S1's `; sframe call` lines carry and `opensnes-rom check --objects` re-derives from
the instructions), **not variadic** (IR). Today's splicer adds: no alloc kept, no section of its own (none among the
eligible), the temporary room. "One level" is the second ROM reading above; the splicer nests (bottom-up), and the rule's
figure is the nested one with its ROM.

Variant **W** (not in the rule): an exported or address-taken callee copied at its direct calls of the unit, its
standalone kept. Jeu 1: 45 callees, 159 executed calls, **0.13 %**, +6.7 KB; jeu 2: 40 callees, 896 calls, **0.71 %**,
+16.9 KB. **The gapbench routine `call` is this variant**: its callee `k_call_f` is global (`devtools/gapbench/kernels.c`;
the benches' workloads are not static, `compiler.md`), so I1 as scoped (static callees) does not move it: the other
compiler's 0.41 there comes from copying a global function into its caller.

### (5) ROM against the free space

`opensnes-rom budget --json` on the `ref1114` ROMs: **885 KB free (jeu 1, 2 MB ROM), 1 581 KB free (jeu 2, 4 MB)**; bank
$00 is full (60 bytes) on both, which no longer fails a link (code is `SUPERFREE`). Up to 256 B nested costs **+68 KB
(8 % of the free space) and +160 KB (10 %)**: it fits. Everything nested does not (+7.5 MB, +2.2 MB); everything one
level does (+40 KB, +81 KB) but gains 1.5 / 1.3 %. Corpus: +24 KB one level, +39 KB nested over 65 ROMs (42 with an
eligible callee; the largest, `games/rooftop_run`, +7.6 KB nested).

### Against the prediction

Predicted 3.4 / 3.5 % (2.4 to 4.7) for every eligible callee, with the ROM "it fits". Wrong twice. **H1 is contradicted
upwards on jeu 1** (97.5 % of the executed same-file calls go to eligible callees, not 85 %; jeu 2 88.6 %) and the saving
per call is at the top of the range (224 and 222 mclk, against 180 in 140 to 220): every eligible callee gives 4.9 /
4.5 %, above my range on jeu 1. **The ROM prediction is wrong**: everything nested does not fit (+7.5 MB). The rule
passes only under a size limit, at 2.55 / 2.35 % for 256 bytes, inside the predicted range for the wrong reason (a limit
I did not foresee halves the figure). The verdict ("above 1.5 %, the lot goes on") holds.

## Prediction of the recompiled check (written before it runs)

The count prices today's text minus the call; this check measures what the compiler does. Private: a copy of the gauge
(its frozen sources, its scenes, its windows aligned on the games' steps, its tick-by-tick equivalence against
`ref1114`), the 147 / 181 eligible callees up to 256 B of the games' own units (the library's are left alone: they are
the SDK's) marked `static inline`, built with `CC_INLINE_MAX_BIG=100000` by the SDK of `ref1114` (no compiler change:
today's splicer copies a `static inline` callee at every site, nested, bottom-up). The unmarked copy was rebuilt first:
**ROMs `cmp`-identical to `ref1114`** on both games (label `i1-temoin`).
- The count for exactly that set: **2.47 % (jeu 1), 2.35 % (jeu 2)** nested, ROM **+64 KB, +159 KB**.
- **My prediction of the measurement**: the equivalence holds (every tick identical to `ref1114`); ROM within 30 % of
  the count, or less where the temporary room declines copies; busy per step down by **1.0 to 2.5 %**, most likely
  **about 1.5 % on each game** (0.6 of the count): the count is blind to what the bigger caller costs (callee temps
  crossing the caller's calls go to the stack, leaf frames lost, the text rules' lines changed), which the 2026-10-10
  measurement saw at -2 % then +0.7 % for the one-site splices; and the same temporary room that declines today's
  one-site callees will decline copies into the big callers. What would contradict it: a measured gain above the count
  (constant propagation, the text rules gaining on the new shapes), or under 1 % (the register part and the declines
  eating most of it).

## Result of the recompiled check (2026-11-14, after the prediction above was committed, `f7f76365`)

**The recompiled text confirms the verdict: 1.80 % (jeu 1) and 1.71 % (jeu 2) of busy time per step, equivalence
identical, +47 KB and +146 KB of code.** Above the 1.5 % rule on both games, measured, not priced.

| | jeu 1 | jeu 2 |
|---|--:|--:|
| callees marked `static inline` (the games' own units, up to 256 B) | 146 | 181 |
| the count for that set (nested, lower bound) | 2.47 % | 2.35 % |
| **measured, busy per step, `i1-temoin` -> `i1-256b`** | **368 101 -> 361 476 mclk (-1.80 %)** | **609 299 -> 598 864 (-1.71 %)** |
| measured / count | 0.73 | 0.73 |
| equivalence, tick by tick against `ref1114` | identical (341 steps) | identical (700 steps) |
| late frames | 156 -> 147 | 417 -> 419 |
| code bytes (`.sym` sections), count's ROM | 115 892 -> 163 069 (+47 177), count +62 285 | 134 396 -> 280 026 (+145 630), count +158 984 |

- The gauge's own windows (aligned on the games' steps, images 330-1010 and 402-1602), not the traces' 4 x 40 frames
  the count used; the private copy of the gauge (its frozen sources copied, then marked) and the SDK of
  `ref1114` (`812a168b` sources, `bin/` of 09:59): the unmarked copy rebuilt **`cmp`-identical** to `ref1114` first.
- The prediction (1.0 to 2.5 %, most likely about 1.5 %; the ROM within 30 % of the count; equivalence identical)
  holds on every point; the measurement is a little above the most likely figure (1.80 / 1.71 against 1.5). The count
  overstates by a quarter, the same on both games; its ROM by a quarter and a tenth (hypothesis, not read: fewer copies
  than counted where the temporary room declines, and copies smaller once the text rules run on them).
- **Found on the way, a loud defect of today's splicer** (not fixed here, no compiler change in this lot): the first
  build failed to link (`Reference to an unknown label "...inl4_hText"`). A cloned block's label is
  `inl%d_%.22s.%d.%.40s` (`compiler/qbe/inline.c`, `splice_fn`): nested three deep, the block name of the inner copy is
  cut at 40 characters, and the cut fell right after `hText.l` (of `hText.lscreens`); WLA reads a trailing `.b` / `.w` /
  `.l` of a label operand as a size suffix (the rule L4 S1 met with `sf.foo.b`), so `jmp @...hText.l` names a label
  that does not exist. Any cut of a function name (`%.22s`) or block name (`%.40s`) that ends in `.b`, `.w` or `.l`
  does it; nothing in today's corpus or games nests deep enough (they build), a lot that copies more will. The run
  above leaves that one callee (cold, 0.000 % in the count) unmarked (`i1-256b`). Fix for the next step: never end a
  cut on a dot and one letter (or end every cloned label with a character WLA does not read as a suffix), with a
  compiler check that nests a `x.b` name three deep.

### What neither the count nor the check measures

- The count prices today's text minus the call; the check measures one set (up to 256 B, the games' units). Neither
  says what a compiler heuristic would choose on other programs, nor the library's 21 eligible callees (left alone).
- No constant propagation in the lower bound; the upper bound (7.6 / 7.2 %) is a hypothesis, and the measurement sits
  far under it.
- RAM: a copy merges the callee's frame into the caller's; the stack depth at the caller's other calls grows. Not
  measured (the gauge's equivalence and late frames are unchanged; the stack floor gate of `rom_coverage.py` is the
  corpus's).
- Compile time with 160-instruction copies raised to 100 000 (`CC_INLINE_MAX_BIG`): not measured.
- The 0.81 / 0.75 % held by one-site callees the splicer declines today: their cause is a hypothesis (the temporary
  room), not read in the source's decision.
- The examples: census only (no trace).
- luna's DRAM-refresh bias of the busy figures (the gauge's notes) is the same on both sides of the check.

### Replay

```
make clean && CC65816_KEEP_IR=$PWD/build/a1_ir make            # the SDK, the library and the corpus with their IR kept
make count-i1 A1_IR=build/a1_ir                                # census of the corpus and of the library
# the two games (private, the gauge's i1s0 folder): run.sh = i1_count.py on ref1114 with the a1s0 IR and traces
#   (--casm-dir <game> --casm-dir lib/build/lorom --ir-dir <game IR> --ir-dir <SDK IR> --rom --sym --trace
#    [--frames A-B] [--write-set 256 PATH] [--set PATH] --json)
# the recompiled check: a copy of the gauge, marque_inline.py <set> <copy>/sources/<game>/game, then
#   CC65816_KEEP_IR unset, CC_INLINE_MAX_BIG=100000 python3 jauge.py --sdk <SDK of ref1114> --etiquette i1-256b
#   --ref ref1114, and jauge.py --compare i1-temoin i1-256b
```
