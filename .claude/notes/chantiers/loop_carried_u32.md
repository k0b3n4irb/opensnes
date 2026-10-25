# Loop-carried u32 values copied every iteration (accepted 2026-10-22; points 1 and 2 done 2026-10-25)

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

Left, in order: point 3 (a loaded value through a stack slot before the
`adc`), point 4 (the u16 -> u32 index extension whose high half nobody
reads), point 5 (`tab[i + 1]` recomputing the index).

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
