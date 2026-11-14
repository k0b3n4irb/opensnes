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
