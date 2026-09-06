# Far RAM Tutorial — FAR objects beyond the 8 KB band {#tutorial_far_ram}

This tutorial covers the `FAR` qualifier: how to place bulk C data in the
56 KB of WRAM above the 8 KB bank-0 band, what it costs, and when not to
use it. It assumes you have read the [Graphics](graphics.md) tutorial;
the [DMA tutorial](dma.md) is useful background because most `FAR`
buffers end up as DMA sources.

## The two RAM bands

The SNES has 128 KB of WRAM, but the compiler's default addressing is
bank-0-implicit: a plain C global must live in `$00:0000-$1FFF`, the
8 KB that also holds the stack and the direct-page registers. Every link
prints how much of that band is left:

```
OK: C RAM band $0000-$1FFF: 1436 bytes free (top at $1A64; 5544 bytes in 38 sections)
```

A tilemap built in RAM (2 KB), a palette buffer (512 bytes) and a brick
grid were enough to leave breakout with 1.4 KB for everything else. The
second band is `$7E:2000-$FFFF`, 56 KB, and `FAR` is how a C object gets
there:

```c
#include <snes.h>

FAR u16 blockmap[0x400];      /* 2 KB tilemap, built in RAM, DMA'd to VRAM */
FAR u8  blocks[100];          /* brick state */
FAR u8  hdma_table[2][1030];  /* double-buffered HDMA table */
```

The same link then prints both bands:

```
OK: C RAM band $0000-$1FFF: 6880 bytes free (top at $0520; 836 bytes in 37 sections)
OK: far RAM band $7E:2000-$FFFF: 52636 bytes free (top at $3264; 4708 bytes in 1 sections)
```

A `FAR` object behaves like any other global. It is zero at boot (crt0
clears the far band with one DMA, 22 ms once), it can carry an
initialiser, and every access to it is compiled with bank-honouring
addressing — the qualifier rides on the type, like `const`, so a
`u8 FAR *p` pointer keeps the property through function calls.

## Rules the compiler enforces

- **Static storage only.** The stack is bank 0, so a `FAR` local is an
  error. `static FAR u8 buf[256];` inside a function is fine.
- **Not `const`.** ROM is already reachable from any bank; `FAR const`
  is refused.
- **No silent narrowing.** Passing or assigning a `T FAR *` to a plain
  `T *` is a compile error, in an initialiser, an argument or a plain
  assignment. A plain pointer converts to `T FAR *` silently and just
  takes the far path.
- **`const` accepts far.** A `T FAR *` converts to `const T *` without a
  cast, because a read through a const pointee is already a far read.
  This is why the whole DMA and asset API takes `FAR` buffers as they
  are:

```c
dmaCopyVram((const u8 *)blockmap, 0x0000, 0x800);   /* u16 -> u8 needs the cast anyway */
dmaCopyCGram((const u8 *)pal, 0, 512);
hdmaSetup(6, HDMA_MODE_2REG_2X, HDMA_DEST_M7A, hdma_table[0]);
```

Casting a `FAR` pointer to a plain pointer explicitly compiles and is the
one way to get a silent wrong-bank access. Do it only when the object
really is in bank 0.

## What it costs

Measured with the `b2_deref` bench (cycles per access, loop overhead
removed):

| Access | bank 0 | `FAR` |
|---|---|---|
| byte load through a pointer walk `p[k]` | 116 | 121 |
| word load through a pointer walk | 132 | 137 |
| byte load `arr[k]` | 107 | 88 |
| byte store through a pointer walk | 88 | 74 |
| byte store `arr[k]` | 79 | 41 |

Indexing a `FAR` array by name is *cheaper* than the bank-0 form: the
backend folds `arr + k` into one absolute-long-indexed instruction
(`lda.l arr,x`) instead of materialising the address first. A pointer
walk stages the 24-bit pointer once in a direct-page pair and then reads
with `[tcc__r9],y`, within 4 % of the bank-0 path. Struct fields through
a `FAR` pointer stage the pointer once per basic block:

```c
void set_rec(far_rec FAR *r, u16 w, u32 l) { r->w = w; r->l = l; }
/* -> one staging, then ldy #2 / sta [tcc__r9],y, ldy #4 / ..., ldy #6 / ... */
```

## When to use it, when not to

Use `FAR` for **bulk data that is read or written in loops and handed to
DMA**: tilemaps built in RAM, palette buffers for fades, HDMA tables,
entity pools, level state. That is exactly what fills the 8 KB band and
exactly what the far forms handle at no visible cost.

Keep in bank 0 the **small hot state that lib helpers walk through a
plain pointer**: `Rect`, `AnimPlayer`, `AudioSample`,
`AudioVoiceState`. Those helpers (`rectContains`, `collideRect`,
`animTick`, `audioGetSampleInfo`, …) take plain pointers on purpose; the
compiler refuses a `FAR` argument there, so nothing goes wrong silently.
Two helpers that legitimately fill a bulk buffer accept either band:
`sramLoad` / `sramLoadOffset` and `dsp1Raster`.

## Worked example: breakout

`examples/games/breakout` builds its two tilemaps and its palette in RAM
and re-uploads them after every brick hit. The buffers are declared in
assembly and reached from C:

```asm
.RAMSECTION ".game_buffers" BANK $7E SLOT 2
    blockmap    dsb $800
    backmap     dsb $800
    pal         dsb $200
    blocks      dsb 100
.ENDS
```

```c
extern FAR u16 blockmap[];
extern FAR u8  blocks[];

static void writenum(u16 num, u8 len, u16 FAR *tilemap, u16 pos, u16 offset);
static void mycopy(u8 FAR *dest, const u8 *src, u16 len);

a = blocks[b];                 /* lda.l blocks,x */
blockmap[0x62 + c] = 13 + (a << 10);   /* sta.l blockmap,x */
dmaCopyVram((const u8 *)blockmap, 0x0000, 0x800);
```

The whole migration was the `FAR` on four `extern` lines, the three local
helpers taking `FAR` pointers, and `(const u8 *)` on the DMA calls. The
ROM renders pixel for pixel the same, and bank 0 went from 1436 to 6880
free bytes.

## Debugging

`symmap.py --check-ram-budget game.sym` prints both bands. In luna, a
`FAR` symbol resolves to its `$7E:xxxx` address from the `.sym` file, so
`--peek far_buf:16` reads it directly. If a value reads as zero or as
garbage where you expect data, check first that no explicit cast dropped
the qualifier on the way — that is the only path the compiler does not
guard.

## See also

- [DMA tutorial](dma.md) — the far buffers' usual destination.
- [HDMA tutorial](hdma.md) — tables built in RAM are the classic case.
- `KNOWN_LIMITATIONS.md` — the bank-0 band entry and the far-band escape.
- `compiler/ABI.md` — the address-space section of the calling convention.
