# Loop-carried u32 values copied every iteration (accepted 2026-10-22, not started)

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
