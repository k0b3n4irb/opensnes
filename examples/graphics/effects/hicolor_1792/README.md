# HiColor — 1792 colors from a 4bpp background

![Screenshot](hicolor_1792.png)

Port of krom (Peter Lemon)'s **HiColor64PerTileRow** demo
([PeterLemon/SNES](https://github.com/PeterLemon/SNES), `PPU/HDMA/HiColor64PerTileRow`).
An H-timer IRQ fires on every scanline and general-DMAs 16 bytes (8 colors)
into CGRAM while the PPU draws; over each 8-line tile row that streams a
fresh 64-color set. Even tile rows render from CGRAM 0-63 while the stream
refills 64-127 for the odd rows, and vice versa — 28 rows × 64 = **1792
palette slots** from a mode that nominally allows 128. HDMA cannot do this:
its widest mode moves 4 bytes per scanline; the technique needs 16.

The sunset art is original (procedural, `res/sunset.png` — 15,885 source
colors); `devtools/hicolor64.py` reimplements krom's converter contract
(64×8-pixel segments quantized to 15 colors + black). 357 distinct colors
land on screen — any static 4bpp screen caps at 128.

## SNES Concepts

- H-timer IRQ (`irqSet` / `irqSetHTimer` / `irqEnable`) — per-scanline interrupts
- Raw ASM IRQ handlers (`irq_stream.asm`) — see the contract in `<snes/interrupt.h>`
- General DMA to CGDATA during active display; DMA source auto-advance across transfers
- CGRAM double-banking via tilemap palette bits (rows alternate palettes 0-3 / 4-7)
- OPHCT/OPVCT 2-read latch discipline (`$213F` read-pointer reset)

## Register fidelity vs the original

| Register | krom | this port | note |
|---|---|---|---|
| BGMODE | `%00001011` | same (`setMode(BG_MODE3, 0x08)`) | Mode 3, BG2 8×8 |
| BG2SC | `$3C` (map $3C00w, 32×32) | same (`bgSetMapPtr`) | map loaded at row 4 ($3C80w) |
| BG12NBA | `$00` | same (`bgSetGfxPtr`) | tiles at $0000 |
| BG2VOFS | 31 | same (`bgSetScroll`, NMI-synced) | aligns rows with reset cadence |
| TM | `%00000010` | same (`setMainScreen(LAYER_BG2)`) | BG2 only |
| DMAP0/BBAD0 | `$00`/`$22` | same | 1 byte → CGDATA, inc source |
| HTIME | 190 | same (`irqSetHTimer(190)`) | writes land in H-blank |
| NMITIMEN | `%10010000` | `%10010001` | we keep auto-joypad (SDK default) |
| IRQ handler | HTIMERIRQ verbatim | + save/restore, `$213F` reset | ours interrupts arbitrary C |
| VBlank rewind | VBLANKIRQ verbatim | C callback via `nmiSetBank` | CGADD=0, 128 B, source reset |

## Measured parity (luna v1.9.0 + Mesen2 cross-check)

- **Stream cadence is cycle-exact in both emulators**: DMA source at
  scanline 100 = `pal + 128 + 100×16` = `$86C0` in luna AND Mesen2
  (in-ROM probe reading `$4302` back).
- All 28 bands render from their own palette row (28/28 best-match).
- Residual vs the static per-row expectation concentrates on the first
  scanline of each band — the palette-race window inherent to the
  technique. Same structure as the reference: krom's ROM in luna diverges
  from its own shipped PNG on exactly those lines (first-line residual
  149.0 on his test pattern vs 16.7 on our smooth gradients; other lines
  3.0 vs 6.5).
- Render is frame-stable (0 differing pixels across consecutive frames).

## Two SDK bugs this port surfaced

1. `consoleInit()` seeded the RNG with single reads of OPHCT/OPVCT,
   leaving the latch read pointers mid-sequence for the whole session —
   every later latched V read returned hi/lo-swapped garbage. Fixed in
   `lib/source/console.c` (STAT78 read after seeding resets the pointers).
2. `nmiSetBank()` restored NMITIMEN from a literal, which would have
   clobbered IRQ enable bits — replaced by the `nmitimen_shadow` scheme
   (all $4200 writes compose through the shadow).

## How to Build

```bash
cd examples/graphics/effects/hicolor_1792 && make
```

To regenerate the assets from different art (requires Pillow):

```bash
python3 ../../../../devtools/hicolor64.py res/sunset.png res/sunset
```

## Modules Used

`console`, `dma`, `background`
