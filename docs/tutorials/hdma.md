# HDMA Tutorial {#tutorial_hdma}

This tutorial covers SNES HDMA (Horizontal-blanking DMA): what it is, when
to reach for it, the four registers per channel, the eight transfer modes,
and the per-scanline patterns the seven shipped examples exercise. It
assumes you have already worked through the [Graphics](graphics.md) and
[Scrolling](scrolling.md) tutorials.

## What HDMA actually is

A regular DMA (often written "MDMA" — *general-purpose* DMA — when you
want to disambiguate from HDMA) moves a block of bytes between WRAM/ROM
and a PPU register, synchronously, at the CPU's request. It happens once
when you trigger it.

HDMA is different in three ways:

1. **Timing** — the PPU's HBlank window between scanlines, every scanline,
   for as long as the channel stays enabled. Not on demand; it just keeps
   running once configured.
2. **Cadence** — you supply a *table* in memory that says "write this
   value(s) for *N* scanlines, then write that value for *M* scanlines,
   then stop". The PPU walks the table as it draws each frame.
3. **Cost** — HDMA reads steal a few machine cycles from each scanline.
   The 65816 still runs your code, just slightly slower during active
   display. You're paying for the per-scanline write.

The classic uses are effects whose look *changes vertically across the
frame*: a colour gradient (top different from bottom), a wave that bends
the picture, a parallax background where each layer scrolls at its own
rate, a window mask that follows the screen down, a brightness fade.
Anything that needs "the value of register X is different on scanline 100
than it was on scanline 50" is reaching for HDMA.

If your effect is the *same* across the whole frame, set the register
once at VBlank and stop. HDMA pays its scanline cost regardless of
whether the value actually changes per line.

## The four channel registers

The SNES has eight DMA channels (0–7). Each channel has the same set of
registers; HDMA configures them slightly differently from MDMA.

For HDMA channel `n` (where `n` is 0–7), the addresses are `$43n0`,
`$43n1`, `$43n2`, `$43n3`, `$43n4`. The lib defines them as `HDMA_CHANNEL_n`.

| Register | Name | Purpose |
|---|---|---|
| `$43n0` | `DMAPn` | Transfer mode (3 bits) + reverse / direct addressing flags |
| `$43n1` | `BBADn` | The PPU register the writes target — the low byte of `$21nn` |
| `$43n2`–`$43n4` | `A1Tn` / `A1Bn` | 24-bit source address of the table (`A1Tn` low+high, `A1Bn` bank) |
| `$420C` | `HDMAEN` | Bitmask: which channels run HDMA this frame |

You set those, set the corresponding bit of `$420C`, and let the PPU drive
the rest.

The lib hides those four register writes behind two helpers:

- **`hdmaSetup(channel, mode, destReg, table)`** — for tables in bank `$00`
  (most cases — RAM, or const data that *fits* in bank `$00`'s 32 KB).
- **`hdmaSetupBank(channel, mode, destReg, table, bank)`** — when the
  table lives in any other bank. Required when const data spilled into
  bank `$01+` (see the gotcha section below).

…and a separate enable/disable pair:

- **`hdmaEnable(channelMask)`** — set bits in `$420C` for the channels you
  want to run.
- **`hdmaDisable(channelMask)`** — clear those bits.

## Transfer modes

The "what does each table entry write" axis. Eight modes, set via the
low 3 bits of `DMAPn`:

| Mode | Bytes per write | Goes to | Typical use |
|:---:|:---:|---|---|
| 0 | 1 | reg | INIDISP brightness, MOSAIC, single-byte registers |
| 1 | 2 | reg, reg+1 | BG scroll (`BGnHOFS` writes `$210D` low then high), CGADD/CGDATA pairs |
| 2 | 2 | reg, reg | Two-byte writes to the **same** register (e.g. CGADD-then-CGADD) |
| 3 | 4 | reg, reg, reg+1, reg+1 | Mode 7 matrix elements |
| 4 | 4 | reg, reg+1, reg+2, reg+3 | Window position pair (WH0L, WH0H, WH1L, WH1H) |
| 5 | 4 | reg, reg+1, reg, reg+1 | Pair-of-2-byte registers |
| 6 | 2 | reg, reg | Same as mode 2, alternate latching |
| 7 | 4 | reg, reg+1, reg+2, reg+3 | Same as mode 4 |

The lib defines named constants — `HDMA_MODE_1REG`, `HDMA_MODE_1REG_2X`,
`HDMA_MODE_2REG`, etc. Picking the right mode is mostly a question of
"what register am I writing, and how many bytes does it want?".

## Table format

Every HDMA table is a sequence of *groups*. Each group starts with a
**line-count byte**, followed by `N × byteCount` data bytes (where
`byteCount` is the bytes-per-write of the transfer mode and `N` depends
on whether the high bit of the line count is set).

The line-count byte's high bit is the **repeat flag**:

- **Bit 7 = 0 — non-repeat mode.** "Write the data once, then hold the
  destination register's value for *N* scanlines." Good for registers that
  *latch* their value across scanlines (CGADD, COLDATA, INIDISP).
- **Bit 7 = 1 — repeat mode.** "Write the data on every one of these *N*
  scanlines." **Required** for registers that the PPU *consumes* every
  scanline (BG scroll registers, Mode 7 matrix, window).

A table ends with a line-count byte of `0`.

Worked example, non-repeat — a gradient that writes one byte per group
to set background colour intensity per stripe:

```c
const u8 gradient_table[] = {
    32, 0x00,    /* 32 scanlines: COLDATA = 0x00 */
    32, 0x08,    /* 32 scanlines: COLDATA = 0x08 */
    32, 0x10,    /* 32 scanlines: COLDATA = 0x10 */
    32, 0x18,    /* 32 scanlines: COLDATA = 0x18 */
    32, 0x20,    /* 32 scanlines: COLDATA = 0x20 */
    32, 0x28,    /* 32 scanlines: COLDATA = 0x28 */
    32, 0x30,    /* 32 scanlines: COLDATA = 0x30 */
    0,           /* End */
};
```

Same effect with repeat mode would write every scanline:

```c
const u8 scroll_wave_table[] = {
    0xA0, 0x10, 0x00,    /* $A0 = $80 | 32 → repeat for 32 scanlines */
                          /* Each scanline writes BGnHOFS = 0x0010    */
    0xA0, 0x20, 0x00,    /* Repeat for 32 scanlines, BGnHOFS = 0x0020 */
    /* ... */
    0,                    /* End */
};
```

The "wrong mode" failure is silent and visible: scroll registers in
non-repeat mode write only the first scanline of each group, then the
PPU's latched scroll value drifts for the rest. You see the effect
"glitching" instead of holding.

## Setup pattern

The full sequence to put HDMA on screen:

```c
#include <snes.h>
#include <snes/hdma.h>

const u8 my_table[] = { /* … as above … */ };

int main(void) {
    consoleInit();
    setMode(BG_MODE1, 0);
    /* … load tiles, palettes, tilemap … */

    /* Configure HDMA channel 6 to write to COLDATA ($2132) */
    hdmaSetup(HDMA_CHANNEL_6, HDMA_MODE_1REG, 0x32, my_table);

    /* Enable channel 6 — HDMA starts firing on the next active display */
    hdmaEnable(1 << HDMA_CHANNEL_6);

    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
}
```

Two things often forgotten:

1. **The destination register argument** is the *low byte* of the PPU
   register address, not the full 16-bit address. `0x32` for `$2132`
   (COLDATA), `0x0D` for `$210D` (BG1HOFS), `0x21` for `$2121` (CGADD),
   etc. See `lib/include/snes/hdma.h` for the named constants
   (`HDMA_DEST_COLDATA`, `HDMA_DEST_BGnHOFS`, etc.).
2. **Enable comes last.** Configure first, then flip `$420C`. Enabling
   before the source/mode/destination are settled gives the PPU a frame
   of garbage HDMA reads.

## Worked patterns (the seven shipped examples)

Every shipped example exercises a distinct HDMA case. Read the
`@par What to Observe` block at the top of each `main.c` for the
interactive demo; the patterns themselves are reusable building blocks.

### Background colour gradient — `examples/graphics/effects/hdma_gradient`

Builds a brightness gradient at runtime via the
`hdmaBrightnessGradient(channel, topBrightness, bottomBrightness)` helper,
which interpolates between the two values across 224 scanlines and emits
an HDMA table writing `$2100` (INIDISP) per scanline group. Press **A**
to cycle gradients; **B** to disable. The 8 bpp Mode 3 background
underneath shows the smooth fade from full brightness at top to dark at
bottom.

### Per-scanline wave distortion — `examples/graphics/effects/hdma_wave`

Pre-computes seven sine tables (224 + 111 wrap entries each) at amplitudes
0–24 pixels, then runs HDMA channel 6 in `HDMA_MODE_1REG_2X` to write
both bytes of `BG1HOFS` ($210D) every scanline. Animation advances the
table pointer by 3 bytes per frame to scroll the wave continuously. The
alternating solid/empty tile pattern in the background makes the
distortion clearly visible.

### Parallax scrolling — `examples/graphics/effects/parallax_scrolling`

A static BG3 with a moving BG1 on top, where BG1 scrolls at one rate at
the top of the screen and a *different* rate at the bottom — classic
two-speed parallax. The HDMA table is regenerated each frame in RAM
(non-const) to update the scroll offsets, written to `BG1HOFS`. This
example is also a real-world demonstration of why HDMA tables that
change every frame want to be in RAM rather than in const ROM (see
"bank byte trap" below).

### Static colour gradient — `examples/graphics/effects/gradient_colors`

A simpler form of the brightness gradient: a fixed table in ROM written
to CGADD/CGDATA changes the BG palette colour on different rows, giving
a banded sky. Useful pattern when the gradient never animates and the
table can live in const ROM.

### Helpers showcase — `examples/graphics/effects/hdma_helpers`

Walks through the lib's HDMA helpers (`hdmaSetup`, `hdmaSetupBank`,
`hdmaEnable`, `hdmaDisable`, the brightness-gradient builder) on a
single screen, with on-screen text labelling each. The example to skim
when you want to remember the API surface.

### Window animation — `examples/graphics/effects/transparent_window`

Combines HDMA with the window/colour-math pipeline. HDMA writes to the
window position registers (`WH0L`, `WH0H`, `WH1L`, `WH1H`) per scanline
to animate a moving window, while colour math blends the BG1 layer
through the window onto the BG2 layer beneath. Mode 4 transfer (4 bytes,
4 sequential registers) does the four window writes in one HDMA group.

### Stationary window — `examples/graphics/effects/window`

The non-animated cousin: HDMA in repeat mode writes to the window
registers once (and then "every scanline" since window registers are
write-only and forget on the next scanline). The window position holds
across the visible region; the effect is a static masked area rather
than animation.

## Gotchas

### 🔴 Channel 7 is reserved for OAM DMA

The runtime's NMI handler uses **DMA channel 7** to transfer the OAM
shadow buffer to the PPU each frame. **Do not configure HDMA on channel
7** — the OAM DMA will overwrite your HDMA register setup, the HDMA will
read garbage, and you will spend an hour wondering why your gradient
disappears every few frames. Use channels 1–6 for HDMA. Channel 0 is
also taken by `dmaCopyVram`, so if you have a hot-loop that calls it,
prefer channels 1–6 for HDMA.

This is enforced by convention, not by code. The lib's documentation
(`lib/include/snes/hdma.h`) names the trap explicitly.

### 🔴 Bank byte trap on `hdmaSetup`

`hdmaSetup` hardcodes bank `$00` for ROM source addresses (≥ `$8000`).
If your table is a `static const u8 mytable[] = …` and bank `$00`'s 32 KB
filled up, the linker spills the table into bank `$01+` — but `hdmaSetup`
still tells the PPU "read from bank `$00`, address X". The PPU happily
reads garbage from wherever bank `$00` address X lands.

Two fixes:

- **`hdmaSetupBank(channel, mode, destReg, table, bank)`** with the
  correct bank byte (typically `:mytable` resolved by the assembler).
- **Use a RAM table.** RAM is in bank `$7E` (or its bank `$00` mirror),
  always at predictable addresses. Animated tables that change per frame
  want to be in RAM anyway, so this often happens naturally.

The bank-overflow check (`make/common.mk` → `symmap.py`) flags when bank
`$00` is filling up; pair that with an explicit choice of `hdmaSetupBank`
when const tables get big.

### 🟡 Repeat-mode discipline matters

Three classes of registers, three rules:

| Register class | Examples | Mode |
|---|---|:---:|
| Self-latching (PPU holds value) | INIDISP, COLDATA, CGADD, MOSAIC | non-repeat OK |
| Per-scanline-consumed | BG scroll, Mode 7 matrix | **repeat required** |
| Write-only / forgets | WH0L/H, WH1L/H | **repeat required** |

Wrong mode for the second/third class shows as "the effect works for the
first scanline of each group, then the latched value drifts". Verify in
Mesen2's tile/window viewer if you suspect this.

### 🟡 HDMA fires before the user callback in the NMI handler

`templates/crt0.asm`'s NMI handler runs the VRAM-critical work (OAM DMA,
tilemap DMA, scroll sync) *first*, then calls your `nmiSet()` callback,
then reads input. HDMA configuration done in the user callback takes
effect *next* frame, not the current one. If you need to reconfigure
HDMA each frame (e.g. to swap tables), do it in main-thread code before
`WaitForVBlank()`.

### 🟡 Don't enable channels you haven't configured

`hdmaEnable(0xFF)` enables every HDMA channel, including channels that
hold leftover register values from a previous configuration (or boot
defaults). Always enable only the channels you've configured this frame:

```c
hdmaSetup(HDMA_CHANNEL_6, …);
hdmaEnable(1 << HDMA_CHANNEL_6);   /* good */

hdmaEnable(0xFF);                  /* bad — channels 0-5, 7 are random */
```

## Cycle cost

HDMA reads steal cycles from the active-display window every scanline.
Approximate budget (FastROM off):

- Per-scanline overhead per active HDMA channel: ~8 cycles for setup,
  plus ~8 cycles per byte transferred.
- A 1-byte non-repeat table running on one channel: ~16 cycles per
  scanline, ~3.5 K cycles per frame.
- A 4-byte repeat table on one channel: ~40 cycles per scanline, ~9 K
  cycles per frame.

The 65816 has roughly 1369 cycles per scanline at 3.58 MHz, so a single
HDMA channel costs 0.6 % – 3 % of CPU time. Two or three active channels
are the practical limit before the cost shows up as missed timing in
inner game-logic loops.

## See also

- `lib/include/snes/hdma.h` — full API reference (function signatures,
  mode constants, destination-register constants).
- `lib/source/hdma.asm` and `lib/source/hdma.c` — implementation.
- [`examples/graphics/effects/hdma_gradient`](../../examples/graphics/effects/hdma_gradient/README.md) — runtime gradient builder.
- [`examples/graphics/effects/hdma_wave`](../../examples/graphics/effects/hdma_wave/README.md) — sine-wave per-scanline scroll.
- [`examples/graphics/effects/hdma_helpers`](../../examples/graphics/effects/hdma_helpers/README.md) — API surface walkthrough.
- [`examples/graphics/effects/parallax_scrolling`](../../examples/graphics/effects/parallax_scrolling/README.md) — RAM table, two-speed parallax.
- [`examples/graphics/effects/gradient_colors`](../../examples/graphics/effects/gradient_colors/README.md) — static ROM gradient table.
- [`examples/graphics/effects/window`](../../examples/graphics/effects/window/README.md) — stationary window via HDMA.
- [`examples/graphics/effects/transparent_window`](../../examples/graphics/effects/transparent_window/README.md) — animated window + colour math.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) — bank `$00` overflow,
  channel-7 reservation, and the runtime traps that bite HDMA work.
- [Scrolling tutorial](scrolling.md) — companion read; HDMA on
  `BGnHOFS`/`BGnVOFS` is the next step beyond the static `bgSetScroll`.
