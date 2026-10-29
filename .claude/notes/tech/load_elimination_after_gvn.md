# A second load elimination after gvn: tried, withdrawn (2026-10-27)

A game reported that `rec_len[i] << 5` read twice in a loop, once in the
condition and once in the body, is loaded and shifted twice. Stand-alone
reproducer (ours):

```c
const u8 *rec_src[64]; u16 rec_slot[64], rec_len[64];
u16 rec_done, rec_head, budget;
void feed(void) {
    while (rec_done != rec_head) {
        u16 i = rec_done & 63;
        if ((rec_len[i] << 5) > budget) break;
        budget -= rec_len[i] << 5;
        vramQueuePush(rec_src[i], rec_slot[i], rec_len[i] << 5, 0x80);
        rec_done++;
    }
}
```

## Why the loads stay

`loadopt` (load.c) runs once, right after SSA construction. There the
three `rec_len[i]` are three address temporaries (`add $rec_len, mul(i,
2)` each time), and `fillalias` gives a sum of a symbol and an unknown
offset the type AUnk with a base of its own: no two of them are known to
be one location, so none is forwarded. gvn merges the addresses later, but
no load pass runs after it.

The third read, after `storew ..., $budget`, would stay even then: QBE
answers MayAlias for two different symbols (alias.c), on purpose, and this
SDK needs it. An assembly file may put two labels on one address and C
may name both through `extern`.

## What was tried

`loadgvn()` in main.c, after the pipeline's gvn: gcm without its sink
(a flag, `gcmnosink`), then fillalias, loadopt, gvn, before the
pipeline's own gcm. Four things had to be learned on the way:

1. Between gvn and gcm the SSA is not valid: gvn keeps one definition per
   value where it first met it, and that definition need not dominate its
   uses until gcm places it. loadopt there died on `ssacheck` (anim.c).
2. After gcm, its sink has given every use of an address its own copy
   (`%snk.N`), and loadopt again sees distinct addresses. Hence a gcm
   without sink first; schedblk must still run (it orders the moved
   instructions).
3. loadopt inserts a load of the slice's size (load.c, `load()`: 1, 2, 4,
   8 bytes give ub, uh, uw, l). A word moved out of a loop came back as
   `loaduh`, which the w65816 emitter refuses; the isel rewrote `loaduh`
   and `loadsh` of class Kw to `loadsw` (a halfword is the word here).
4. 32-bit values carried in a phi broke the emitter's Kl high-half
   invariant: 351 of 2000 difftest_stmt seeds refused to build. Limiting
   the second pass to Kw loads (`loadwordonly`) fixed that.

The reproducer went from 80 to 72 instructions: the second load and shift
gone, `rec_done` kept across the loop in a phi. All 60 examples rendered
the same frames and bench/libbench did not move.

## Why it was withdrawn

With Kw loads only, `difftest_stmt --seeds 1-300` still had three bad
seeds, two of them **silent wrong results**:

- seed 16 program 6, seed 87 program 9: a wrong checksum, both programs
  using `FAR` arrays (`wrong: a0[0] = 0x00000000, expected 0x00000001`);
- seed 234: `copy.c:385: phieq: Assertion pa->narg == pb->narg`.

A pass that miscompiles in silence is not shipped for a 10 % gain on one
loop. The likely leads, not followed: the alias of a far access (the bank
travels in the pointer, and the w65816 lowering of FAR loads may not be
what fillalias assumes), and a phi that loadopt builds with a predecessor
count gcm's moves changed.

## To pick it up

The prototype diff (148 lines, compiler/qbe) was not kept in the tree;
the four points above are enough to rebuild it. Before any new attempt:
reduce seeds 16/6 and 87/9 (`difftest_stmt.py --seeds 16 --keep`), find
which forwarded load gives the wrong value, and run 2000 seeds of both
differential tests with zero failures before anything else.
