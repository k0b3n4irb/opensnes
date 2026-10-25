# Loop-carried u32 values copied every iteration (accepted 2026-10-22; points 1 to 4 done 2026-10-25, point 5 refused)

Requested by a second game (direct message, 2026-10-22): a 16.16 path loop
in u32 costs about 84 instructions per iteration. Reproduced on `develop`
03a0a38a with this stand-alone case (`bin/cc65816 -I lib/include`):

```c
#include <snes.h>
static const u32 tab[512] = { 1, 2, 3 };
u32 gx, gy, gang, grot, gacc;
void f(u16 n) {
    u32 x = gx, y = gy, ang = gang, rot = grot, acc = gacc;
    u16 i;
    while (n--) {
        i = (u16)(ang >> 16) << 1;
        y += tab[i];
        x += tab[i + 1];
        ang = (ang + rot) & 0x00FFFFFF;
        rot += acc;
    }
    gx = x; gy = y; gang = ang; grot = rot;
}
```

What the loop body wastes, in planned order:

1. **The phi moves**: 16 `lda/sta` at the end of each iteration for four
   u32 values. `coalescephi` (`compiler/qbe/w65816/emit.c`) only gives the
   phi result a slot, and slot colouring never puts a phi argument in the
   same slot as its result. The fix is to prefer that slot when the two
   live ranges do not interfere, so the move becomes a self-copy and is
   skipped.
2. **`and.w #65535` on the low word**: this is the identity and should
   not be emitted.
3. **A loaded value goes through a stack slot** (`lda.l tab,x / sta t /
   lda t / clc / adc`) instead of feeding the `adc` directly.
4. **The u16 → u32 extension of the index** (`lda.w #0 / sta n+2,s`):
   the high half is never read.
5. **`tab[i + 1]` recomputes the index** instead of reading `tab+4` with
   the same X.

**Done 2026-10-25 (QBE f259a21): points 1 and 2, 83 -> 62 instructions.**
Not the phi coalescing itself: the slot colouring already let a phi share
its argument's slot. What stopped it was the edge every 32-bit result had
with its operands (`slot_edge(t, arg)` in `color_slots`), because a result
half over an operand overwrites a word it has yet to read. Exact overlap is
safe for add, sub, and, or, xor and copy (each word of the operands is read
before the same word of the result is written), so those pairs became
exact-or-disjoint partners, like a phi and its argument, and prefer each
other's slot. The first try stopped the library build on the slot-ownership
check (textPrint, `str += n; c = *str;`): the add was emitted into str's
slot and also folded into the next load, which read str + n + n. A result
that leads to a load's or store's address, through adds and copies, is
excluded (fixture `ptr_advance_fold`). The `and` with a constant half of
$FFFF skips that half, and both its instructions when the result is in its
operand's slot (point 2). Fixture `loop_carried_u32`; 12 ROMs of the corpus
change, all MATCH frame for frame; bench and libbench unchanged.

Measured by the second game on its real case the same day (same ROM and
scenario, luna profile, before = develop 2087781e): `path_move` 102 532 ->
81 236 master cycles per step over frames 200-1400 (-21 %), median step
387 376 -> 364 188, lag frames 339 -> 321 of 1 200; in its worst window
(frames 4500-7000) 120 866 -> 95 784, steps over three frames 255 -> 221 of
about 710. Same game state at steps 300, 700 and 1500. It still needs about
10 % in that window to hold three frames a step: points 3 to 5 are wanted.
The first game's ROM shrank by 34 bytes, its 27 tests pass, load unchanged.

**Points 3 and 4, 2026-10-25 (QBE ad9fe6c): 62 -> 48 instructions.** A
32-bit far load whose one use is the 32-bit add right after it is emitted
inside the add (`try_fuse_kl_load_add`): the value goes from the table to
the `adc`. And a multiply by a constant power of two passes the "high half
dead" mark to its operand (`is_pow2_mul` in `mark_addr_only_kl`), so the
widened index no longer writes `lda #0 / sta hi`. 18 ROMs change, all MATCH;
the audio hash of snesmod_music moved (`luna diff --audio --align-onset`,
its input script: MATCH, max 0.74 %, onset unchanged at 37726).

Measured by the two games on their real cases (before = develop with
points 1 and 2): the second game's `path_move` 81 236 -> 68 110 cycles per
step over frames 200-1400, lag frames 321 -> 297 of 1 200; in its worst
window 95 784 -> 80 181 (-34 % since the start of the chantier), steps over
three frames 221 -> 191 of about 720; game state identical. The first
game's ROM 149 bytes smaller, 27 tests pass, one tick of three frames less.

**Point 5 refused, for now.** Reading `tab[i + 1]` as `tab + 4` with the
same X means rewriting `(u32)(u16)(i + 1) * 4` as `(u32)i * 4 + 4`, which is
exact only when `i + 1` does not wrap at 65 536. Nothing bounds `i` here (it
is `(u16)(ang >> 16) << 1` with `ang` carried by the loop): the same rule as
the `i + 8` fold of `cdcc4580`. A game that knows its bound writes
`tab[i] ... (&tab[i])[1]`, or a struct of two.

**Left: dead stores to stack slots** (4 of the 48 in this loop: a value
stored and handed on in A, never read from its slot). The flow pass
(`peephole_flow`) tracks the sixteen direct-page words only; this
function's frame is on the stack. Extending it to `N,s` slots is its own
piece of work (offsets move with every push).

The request passes the four checks of `exchanges.md`. This is a Class A
chantier, in a separate worktree, after the assets lot. The finished
commit and the full rebuilds are announced to the game.

The same message asked for a VBlank callback that also runs on lag frames.
The answer is yes, opt-in, later, following the precedent of
`GSU_PRESENT_ON_LAG_FRAMES`. It would be a separate callback on the lag
branch of `NmiHandler`; `nmiSet` keeps its current meaning. The
constraints the header must state:

- the callback reads only a buffer that is already ready;
- it uses its own DMA channel and never a lib DMA helper, because the
  main thread may be setting up channel 0;
- it never DMAs `oamMemory` while the main thread is writing it.

This is Class A (crt0 and `interrupt.h`).

**Thread state (2026-10-22, game's reply).** The game agrees with the
order and with having no date. It considers the lag-callback question
closed and has noted constraints (a) and (b). When the loop commit is
announced, it will measure its real case with the same scenario (luna
profile, frames 200 to 1400) and send us the before/after figures. The
loop thread stays open until those figures arrive.

## Next in the compiler queue: `p[i]` through a far pointer (accepted 2026-10-23)

The game sent a stand-alone case. On develop `e8374a97`, the two chained
reads `lv->w[lv->order[i]]` through far pointers held in a ROM struct cost
74 instructions. Two places waste the most:

1. The high word of the offset `i * 2` takes 9 instructions:
   `sta / asl / sta / lda / xba / and #$00FF / lsr x7`. Right after `asl`,
   the carry already holds that bit, so `lda #0 / rol` would do it in 2.
2. A 32-bit add follows, plus a copy of the pointer into `tcc__r9`, before
   `lda [tcc__r9]`. With the pointer already on the direct page,
   `ldy offset / lda [dp],y` would do the whole read. The compiler already
   uses that form for struct fields.

Accepted, Class A, in the compiler worktree, after the loop chantier:

1. **The carry for the high word.** A local change with no risk.
2. **`[dp],y`, only where the offset is proved below $10000.** With `i`
   a u16 and 2-byte elements, `i * 2` can need 17 bits, and Y carries 16.
   Without a proved bound the 32-bit add stays. This is the same rule as
   the `i + 8` fold (`cdcc4580`).

Whether `[dp],y` carries into the bank byte is a hardware claim. The game
found no arbiter stating it, so it gets arbitrated against the corpus
(`hardware_claims.md`), or measured on luna, before any codegen relies on it.

The thread closes when the reduced case is well under 74 instructions and
the game has measured its real case.

The game's figures for the loop thread (worst case, 2026-10-23):

- a step costs about 0.87 M master cycles (median);
- `path_move` costs about 124 000 on average, up to about 200 000. That
  is the largest item left.
