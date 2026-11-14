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
