# DMA Tutorial {#tutorial_dma}

This tutorial covers SNES DMA — the foundational mechanism for moving
bytes between WRAM/ROM and the PPU. It's the most-used facility in the
SDK (every example with graphics goes through `dmaCopyVram`), and the
gotchas it carries (VBlank discipline, channel reservations, bank-byte
trap, the 4 KB-per-frame budget) come up over and over.

If you've already read the [HDMA tutorial](hdma.md), the channel /
register architecture will look familiar — the DMA registers are the
same physical channel registers, used in a different mode. Read DMA
first, then HDMA: HDMA is a specialisation that streams data per
scanline, while DMA is the bulk-transfer baseline.

## What DMA actually is

A DMA transfer moves a contiguous block of bytes between WRAM/ROM and a
PPU register, off-CPU. The CPU stalls for the duration of the transfer
(it does not keep running), but the per-byte cost is far below what a
hand-rolled `lda`/`sta` loop would manage:

- **Loop in CPU code**: an `lda abs,x` / `sta abs` / `inx` / `bne` loop is
  about 15 CPU cycles per byte, i.e. roughly 90 master cycles per byte at
  FastROM speed (6 master cycles per CPU cycle).
- **DMA**: 8 master cycles per byte, whatever the memory speed, plus 8
  master cycles of overhead per channel and 12-24 for the whole transfer
  (anomie's timing and register docs) — about ten times faster per byte.

The faster per-byte rate plus the lack of loop overhead means DMA is
strictly better for any transfer over a handful of bytes. The lib uses
DMA for every VRAM, CGRAM, and OAM update.

## DMA vs HDMA

The same physical channels and registers serve both modes. The
difference is *when* the transfer fires:

| | DMA (sometimes "MDMA" — *general-purpose* DMA) | HDMA |
|---|---|---|
| Trigger | Synchronous, on `$420B` write (you say "go") | Per-scanline, automatic, gated by `$420C` enable bits |
| Use | Bulk transfers (load tile data, palette, full OAM) | Per-scanline effects (gradients, parallax, perspective) |
| Cost | One CPU stall per transfer | Continuous overhead while enabled |
| Window | Force blank (any time) or VBlank (small budget) | Active display only |

A typical project uses DMA for *setup* (load assets at boot, refresh
OAM each frame) and HDMA for *effects* during display.

## The DMA channels

The SNES has eight DMA channels, numbered 0–7. They share a single
register set per channel (`$43n0`–`$43nA`) but are otherwise
independent — each can run its own transfer. The lib reserves two of
them:

| Channel | Reserved for | Notes |
|:---:|---|---|
| **0** | Main-thread DMA helpers (`dmaCopyVram`, `dmaCopyCGram`, `dmaFillVRAM`, etc.) | Used every time you call those functions. Re-using channel 0 for HDMA collides with these calls. |
| **1–6** | Free for user use | This is where HDMA goes. |
| **7** | Every DMA of the NMI handler | The OAM upload (when `oam_update_flag` is set), the text tilemap flush and the dynamic sprite uploads. The NMI handler reprograms the channel each time, so **do not configure HDMA on channel 7** — see the warning at the top of `lib/include/snes/hdma.h`. |

If you use only the lib's helpers, you never touch the channel
registers directly. If you write a custom DMA path (Mode 7 interleaved
load, multi-channel HDMA, etc.), pick channels 1–6.

## The 8 transfer modes

The low 3 bits of the `DMAPn` register pick how the source bytes are spread
over the PPU registers: one register, two in turn, a write-twice register…
The eight patterns are in one table, under `$43x0 - DMAPx` in
[the register reference](../hardware/REGISTERS.md); HDMA uses the same
modes.

The lib's `dmaCopyVram` uses mode 1 (16-bit VRAM word writes via
`$2118`/`$2119`); `dmaCopyCGram` uses mode 0 with destination `$2122`
(CGDATA). You rarely need to think about modes unless you're writing a
custom DMA helper.

## Direction and source addressing

Two more bits of the `DMAPn` register matter:

- **Direction** (bit 7): 0 = CPU→PPU (the common case), 1 = PPU→CPU
  (rare — used for reading VRAM back, e.g., capturing screenshots).
- **Source addressing** (bit 3): 0 = auto-increment after each byte (the
  common case — march through your source array), 1 = fixed address
  (write the same value repeatedly — used by `dmaFillVRAM` to fill VRAM
  with a single value).

The lib's helpers set these correctly for their use case. Custom paths
need to set them in `$43n0`.

## When VRAM/CGRAM/OAM writes are safe

This is the most important rule in the entire SNES API surface, and the
one that catches the most newcomers:

> **The PPU only accepts writes to VRAM, CGRAM, and OAM during VBlank
> or while the screen is in force blank** (`INIDISP` bit 7 set).
> Outside those windows, writes are silently dropped.

Two windows, two patterns:

1. **Force blank** (`setScreenOff()`) — any time, any duration.
   Used at boot to load all assets, and during gameplay if you need to
   transfer more than ~4 KB at once. The screen goes black while it's
   set; users see this as a "loading flash" if it takes more than a
   frame.
2. **VBlank** (37 scanlines on NTSC, 50,468 master cycles, minus the NMI
   handler's own work; see [Measured frame costs](../PERF.md)) — automatic, every
   frame. Budget is small (~4 KB of DMA). The lib's NMI handler uses this for
   the OAM DMA and the optional tilemap-streaming path.

`isInVBlank()` returns 1 while the PPU is in VBlank (it reads the VBlank
flag of `HVBJOY`, `$4212` bit 7 — anomie's timing doc, S-CPU registers):
a cheap assertion for code that must only run there. Force blank is a
separate question, which `isInVBlank()` does not answer.

A `dmaCopyVram()` call outside both windows produces a build that
*compiles, links, and runs* with no error, then displays garbage tiles
or nothing at all. The catch is silent. `KNOWN_LIMITATIONS.md` flags
this as the canonical 🔴 silent-corruption mode.

## The setup pattern

Bulk asset load at boot, the most common DMA use:

```c
#include <snes.h>

extern u8 tileset[], tileset_end[];
extern u8 palette[];
extern u8 tilemap[];

int main(void) {
    consoleInit();

    /* Force blank for the asset DMAs — the screen stays black until
     * we explicitly turn it on at the end. consoleInit() leaves us in
     * force blank already, so this is documentation rather than a
     * required call here. */
    setMode(BG_MODE1, 0);

    /* Each DMA call writes via channel 0 (the lib's reserved channel) */
    dmaCopyVram(tileset, 0x2000, tileset_end - tileset);   /* tiles */
    dmaCopyCGram(palette, 0, 32);                          /* 16 colours */
    dmaCopyVram(tilemap, 0x0400, 2048);                    /* 32x32 tilemap */

    bgSetMapPtr(0, 0x0400, SC_32x32);
    bgSetGfxPtr(0, 0x2000);
    setMainScreen(LAYER_BG1);

    /* Now the screen turns on — everything we DMAed during force blank
     * is in VRAM/CGRAM, and the tilemap and tile pointers are set. */
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
}
```

The init order — `consoleInit` → `setMode` → DMAs → `bgSet*Ptr` →
`setMainScreen` → `setScreenOn` — is the canonical pattern; getting it
wrong produces a black screen or garbage on the first frame.

## Per-frame DMA in the main loop

For game state that mutates during play (tilemap streaming as the
camera scrolls, palette cycling, dynamic sprite tile uploads), the DMA
happens every frame inside VBlank:

```c
while (1) {
    /* Game logic — accumulates a small DMA payload */
    update_camera();
    update_palette_cycle();

    WaitForVBlank();   /* Halt until VBlank fires */

    /* We're in VBlank now — small DMAs are safe here. Budget ~4 KB. */
    dmaCopyVram(camera_strip_tiles, vram_addr, 64);
    dmaCopyCGram(palette_buffer, 0, 32);
}
```

Two non-obvious points:

1. **`WaitForVBlank()` returns *after* the NMI handler ran** — meaning
   most of the VBlank is still ahead of you: the NMI handler takes
   11-16 % of it (see `docs/PERF.md`). Plan your DMA budget accordingly.
2. **The NMI handler also does DMA** during the same window: the OAM
   DMA (~544 bytes) and any tilemap streaming. That work counts against
   the 4 KB budget too.

If a per-frame transfer exceeds ~4 KB, split it across multiple
VBlanks (the "1-page-per-VBlank" pattern documented in
`KNOWN_LIMITATIONS.md`). For very large updates that *can* tolerate a
visible flash, drop into force blank:

```c
setScreenOff();
dmaCopyVram(huge_tileset, 0x4000, 16384);   /* 16 KB in one shot */
setScreenOn();
```

## Several small uploads in one VBlank: the queue

A game that streams graphics while it plays — a sprite's new frame, the
row or column of map a scroll uncovered — makes several small transfers in
the same VBlank. Each `dmaCopyVram()` call spends more on its arguments
and its setup than on a 128-byte transfer itself, and all of it in the
VBlank, the one stretch of the frame where time is short.

`<snes/vramqueue.h>` (module `vramqueue`) splits the work in two. During
the frame the game **notes** each transfer; in VBlank one assembly routine
**sends** them all:

```c
#include <snes/vramqueue.h>

/* during the frame */
if (!vramQueuePush(frame_tiles, 0x0000, 128, VRAM_QUEUE_ROW)) {
    /* the queue is full: keep this one for the next frame */
}

WaitForVBlank();
vramQueueFlush();          /* first thing in VBlank */
```

The queue holds 32 entries; a project that needs more says so in its
Makefile, `VRAM_QUEUE_MAX := 64` (1 to 255, 10 bytes of the C RAM band
each). The VBlank does not grow with it: `vramQueueCost()` says what the
entries waiting will take.

`VRAM_QUEUE_COLUMN` steps 32 words after each word, for a column of a
32-wide tilemap.

A streamed sprite frame is several strips — one per row of tiles, 512
bytes apart in the sheet `opensnes-sprite sheet` writes and 256 words
apart in VRAM. `vramQueuePushSprite(src, addr, size_px)` notes them all
in one call (two strips for 16x16, four for 32x32), or none if they do
not all fit.

Be clear about what it buys. Six 128-byte transfers, measured
(`devtools/libbench`):

| | Whole cost | Of which in VBlank |
|---|---:|---:|
| six `dmaCopyVram()` calls | 16,600 | 16,600 |
| six `vramQueuePush()` and one `vramQueueFlush()` | 21,700 | 10,700 |

The queue costs **more** in total — noting an entry is a call too — and a
third **less** in VBlank. Use it when the VBlank is what you are short of;
for one or two uploads, or in force blank, call `dmaCopyVram()`.

The budget stays yours: what does not fit in the VBlank lands in active
display and is ignored by the PPU, with no error.

When several modules fill the queue — a status bar, a message box, then
the sprite frames — the last one needs to know what the others already
took. `vramQueueCost()` says it: the bytes queued plus
`VRAM_QUEUE_ENTRY_COST` (96) for each entry, the flush's cost per entry
expressed in bytes. Multiply by 8 for master cycles. The flush sets it
back to 0.

```c
/* sprites go last: they take what the VBlank has left */
while (next < wanted
       && vramQueueCost() + 512 + 4 * VRAM_QUEUE_ENTRY_COST <= SPRITE_BUDGET) {
    vramQueuePushSprite(frame_src[next], frame_dst[next], 32);   /* 4 strips */
    next++;
}
```

The queue does not refuse an entry past a budget: which module gives way
is the game's choice, and the order in which they push is not.

## Lib API tour

| Function | What it does |
|---|---|
| `dmaCopyVram(src, vramAddr, size)` | Copy bytes from WRAM/ROM to VRAM. The bank is taken from `src`'s own bank byte, so the source may live in any bank. Mode 1 (write `$2118`/`$2119`). The workhorse. |
| `dmaCopyVramBank` | **Removed**. `dmaCopyVram` reads the bank from `src`. |
| `dmaFillVRAM(value, dest, size)` | Fill a VRAM region with a fixed 16-bit value (fixed-source mode). Used to clear tilemaps. |
| `dmaClearVRAM(void)` | Zero all 64 KB of VRAM. Boot-time use only (force blank required). |
| `dmaCopyCGram(src, startColor, size)` | Copy palette data to CGRAM. Mode 0 (write `$2122`). |
| `dmaCopyCGramBank` | **Removed**. Use `dmaCopyCGram`. |
| `dmaCopyOam(src, size)` | One-shot OAM transfer, write `$2104`. Mostly used at init — the NMI handler does the per-frame OAM DMA automatically. |
| `dmaCopyVramMode7(tilemap, mapSize, tiles, tilesSize)` | Two-pass interleaved DMA for Mode 7's split low-byte/high-byte VRAM layout. See the [Mode 7 tutorial](mode7.md). |
| `dmaTransfer(channel, mode, src, destReg, size)` | Generic DMA — pick your own channel, mode, destination register; the bank comes from `src`. Use when the named helpers don't fit (e.g., transfers to `$2180` WRAM data port, or experimental modes). |

## Gotchas

### 🔴 VRAM/CGRAM/OAM writes outside VBlank or force blank are silently dropped

Put per-frame transfers after `WaitForVBlank()` and wrap heavy bulk
transfers in `setScreenOff()`/`setScreenOn()`; a write during active
display is ignored with no error. Detail in
[`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md).

### 🔴 VBlank DMA budget is ~4 KB per frame

VBlank is 37 lines (50,468 master cycles) and the NMI handler uses part
of it; past about 4 KB, force blank or split the update across several
VBlanks. Detail in [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md).

### 🟢 SUPERFREE assets in any bank just work

`dmaCopyVram(src, vramAddr, size)` takes the source bank from `src`'s
own bank byte, so an asset the linker placed in bank `$01` or higher
(because bank `$00`'s 32 KB ROM filled up) transfers correctly with no
extra work.

### 🟠 Channel 0 vs HDMA on channel 0

`dmaCopyVram` and friends use channel 0. If you also configure HDMA on
channel 0 (which the lib documents as discouraged but doesn't actively
prevent), the next `dmaCopyVram` call clobbers the HDMA setup. Same
class of bug as the OAM-DMA-on-channel-7 trap — different channel.

**Mitigation**: HDMA on channels 1–6.

### 🟠 OAM DMA is automated, don't double-up

The lib's NMI handler triggers an OAM DMA every frame *when
`oam_update_flag` is set*.
You don't need to call `dmaCopyOam()` per frame — the runtime does it
for you. Use `dmaCopyOam()` only at init (before the first
`WaitForVBlank`), or for one-off forced refreshes.

If you write directly to `oamMemory[]`, set `oam_update_flag = 1`
yourself. The lib's `oamSet`, `oamSetXY`, `oamClear`, etc. and the
`oamSetFast` / `oamSetXYFast` macros do this for you. See
`KNOWN_LIMITATIONS.md` (Performance traps) for the canonical pattern.

### 🟡 Mode 7 needs `dmaCopyVramMode7`, not `dmaCopyVram`

Mode 7 stores tilemap and tile data interleaved in the low and high
bytes of each VRAM word. A single `dmaCopyVram` writes the same source
byte to both halves — wrong. Use `dmaCopyVramMode7(tilemap, tilemapSize,
tiles, tilesSize)` which performs two passes with the right
`VMAIN`/`VMDATAL`/`VMDATAH` configuration, or write the equivalent
assembly helper. See the [Mode 7 tutorial](mode7.md).

### 🟢 WRAM data port `$2180`–`$2183` in an NMI callback (caught at build time)

An NMI that uses `$2180-$2183` in the middle of a main-loop sequence loses
its address. The library's NMI handler never touches them, and the build
refuses a `nmiSet()` callback that can reach them (`opensnes-rom check`).
See [Known limitations](../../KNOWN_LIMITATIONS.md).

## Cycle cost

DMA is the fastest bulk transfer the SNES offers. Approximate cost
per transfer:

- Setup overhead: ~12 cycles (writing `DMAPn`, `BBADn`, `A1Tn`/`A1Bn`,
  `DASn`).
- Per-byte transferred: ~8 master cycles (mode 1 / mode 0).
- MDMAEN trigger: ~8 cycles.

A 1 KB DMA: 12 + 1024 × 8 + 8 ≈ 8,212 cycles, ~2.3 % of a 60 Hz frame
(357,368 master cycles).

A 4 KB DMA: 12 + 4096 × 8 + 8 ≈ 32,800 cycles, ~9.2 % of a 60 Hz frame
(and right at the VBlank budget — only works during force blank or with
careful timing).

The lib's NMI handler skips the 544-byte OAM DMA on frames where
`oam_update_flag` is clear, so a sprite-idle frame does not pay its
~4,300 master cycles.

## See also

- `lib/include/snes/dma.h` — full API reference (function signatures,
  parameter docs).
- `lib/source/dma.asm` — the assembly implementation.
- [HDMA tutorial](hdma.md) — the per-scanline cousin; required reading
  for any per-scanline effect work.
- [Mode 7 tutorial](mode7.md) — the interleaved-VRAM use case for
  `dmaCopyVramMode7`.
- [Scrolling tutorial](scrolling.md) — tilemap streaming, the common
  per-frame DMA payload.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) —
  VRAM-during-active-display silent drop (🔴), VBlank DMA budget (🔴),
  bank `$00` overflow (🟢 caught at link time), WRAM data port NMI
  race (🟢 caught at build time).
