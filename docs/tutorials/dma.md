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

- **Loop in CPU code**: ~10 master cycles per byte (8-bit `lda` / `sta`
  with register-mode register write).
- **DMA**: ~7 master cycles per byte (8 cycles per byte for the lib's
  default mode, slightly less in optimal cases).

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
| **7** | NMI handler's OAM DMA | Pre-armed at boot to OAM-DMA params; the NMI handler triggers it conditionally on `oam_update_flag`. **Do not configure HDMA on channel 7** — see `KNOWN_LIMITATIONS.md` for why. |

If you use only the lib's helpers, you never touch the channel
registers directly. If you write a custom DMA path (Mode 7 interleaved
load, multi-channel HDMA, etc.), pick channels 1–6.

## The 8 transfer modes

The same modes as HDMA — the `DMAPn` register's low 3 bits encode them.
For DMA, the mode determines how the source is written to a PPU register
*sequentially* (vs HDMA's per-scanline cadence):

| Mode | Bytes per "unit" | Pattern |
|:---:|:---:|---|
| 0 | 1 | reg |
| 1 | 2 | reg, reg+1 (the common case for VRAM word writes via `$2118`/`$2119`) |
| 2 | 2 | reg, reg (same register twice — for CGRAM via `$2122`) |
| 3 | 4 | reg, reg, reg+1, reg+1 |
| 4 | 4 | reg, reg+1, reg+2, reg+3 |
| 5 | 4 | reg, reg+1, reg, reg+1 |
| 6 | 2 | reg, reg |
| 7 | 4 | reg, reg+1, reg+2, reg+3 |

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
2. **VBlank** (~2,200 cycles per frame after NMI) — automatic, every
   frame. Budget is small (~4 KB). The lib's NMI handler uses this for
   the OAM DMA and the optional tilemap-streaming path.

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
`setMainScreen` → `setScreenOn` — is the canonical pattern. See the
[`new_example.md` rule](../../.claude/rules/new_example.md) for the
full checklist; getting it wrong produces a black screen or garbage
on the first frame.

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
   you have ~2,200 cycles before the *next* VBlank starts. Plan your
   DMA budget accordingly.
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

## Lib API tour

| Function | What it does |
|---|---|
| `dmaCopyVram(src, vramAddr, size)` | Copy bytes from WRAM/ROM (bank `$00`) to VRAM. Mode 1 (write `$2118`/`$2119`). The workhorse. |
| `dmaCopyVramBank(src, bank, vramAddr, size)` | Same, with explicit source bank. **Required** when source data lives in bank `$01+` (SUPERFREE spillover). |
| `dmaFillVRAM(value, dest, size)` | Fill a VRAM region with a fixed 16-bit value (fixed-source mode). Used to clear tilemaps. |
| `dmaClearVRAM(void)` | Zero all 64 KB of VRAM. Boot-time use only (force blank required). |
| `dmaCopyCGram(src, startColor, size)` | Copy palette data from bank `$00` to CGRAM. Mode 0 (write `$2122`). |
| `dmaCopyCGramBank(src, bank, startColor, size)` | Same, with explicit source bank. |
| `dmaCopyOam(src, size)` | One-shot OAM transfer, write `$2104`. Mostly used at init — the NMI handler does the per-frame OAM DMA automatically. |
| `dmaCopyVramMode7(tilemap, mapSize, tiles, tilesSize)` | Two-pass interleaved DMA for Mode 7's split low-byte/high-byte VRAM layout. See the [Mode 7 tutorial](mode7.md). |
| `dmaTransfer(channel, mode, srcBank, srcAddr, destReg, size)` | Generic DMA — pick your own channel, mode, destination register. Use when the named helpers don't fit (e.g., transfers to `$2180` WRAM data port, or experimental modes). |

## Gotchas

### 🔴 VRAM/CGRAM/OAM writes outside VBlank or force blank are silently dropped

The first thing to internalise. The PPU returns silently from a write
to `$2118`, `$2119`, `$2122`, `$2104`, etc. when active display is on.
There is no error, no flag, no diagnostic. The next time you turn the
screen on, the writes you "did" weren't.

**Mitigation**: every DMA-touching function in the lib calls into a
DMA register sequence that assumes VBlank or force blank. The
discipline is yours: wrap heavy bulk transfers in
`setScreenOff()`/`setScreenOn()`, and put per-frame transfers after
`WaitForVBlank()`.

### 🔴 VBlank DMA budget is ~4 KB per frame

NMI runs for ~35,000 master cycles after VBlank starts (the lib's NMI
handler reserves some of that for OAM DMA, scroll sync, joypad read,
user callback, etc.). DMA costs ~8 cycles per byte. Net: about 4 KB
of *user* DMA before the PPU starts sampling registers for active
display while you're still writing.

Exceed it and the PPU starts reading mid-DMA — you get garbage tiles
on the top scanlines for that frame. The corruption is *visible*, not
silent — but it's also intermittent and hard to debug because it
depends on game state.

**Mitigation**: count your bytes. If a per-frame update needs > 4 KB,
either drop into force blank (visible flash, OK for boss intro / scene
transition) or split across multiple VBlanks ("1-page-per-VBlank"
pattern — `KNOWN_LIMITATIONS.md` documents the canonical form).

### 🔴 `dmaCopyVram` hardcodes source bank `$00` — SUPERFREE spillover trap

The lib's `dmaCopyVram(src, vramAddr, size)` puts the source bank byte
to `$00` in the DMA setup. If the linker placed your asset in bank
`$01` or higher (because bank `$00`'s 32 KB ROM filled up — see
`.claude/rules/bank0_budget.md`), the DMA reads the *wrong* bank and
writes garbage to VRAM.

**Mitigation**: `dmaCopyVramBank(src, bank, vramAddr, size)`.
The bank byte for an `extern` symbol resolves at link time as
`:symbol`, so:

```c
extern u8 huge_tileset[];
/* `:huge_tileset` is the linker-resolved bank — assembly only.
 * From C, you typically build a small assembly helper that knows
 * the bank, like the asm_loadSkyData() helper in
 * examples/graphics/backgrounds/mode7_perspective. */
```

Most projects either (a) keep all assets in bank `$00` (small games),
or (b) write a small assembly DMA helper per asset to handle the
correct bank. The shipped example that hits this case
(`mode7_perspective`) takes option (b).

### 🟠 Channel 0 vs HDMA on channel 0

`dmaCopyVram` and friends use channel 0. If you also configure HDMA on
channel 0 (which the lib documents as discouraged but doesn't actively
prevent), the next `dmaCopyVram` call clobbers the HDMA setup. Same
class of bug as the OAM-DMA-on-channel-7 trap — different channel.

**Mitigation**: HDMA on channels 1–6.

### 🟠 OAM DMA is automated, don't double-up

The lib's NMI handler triggers an OAM DMA every frame *when
`oam_update_flag` is set* (since the perf fix in commit `6d438e6`).
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

### 🟡 WRAM data port `$2180`–`$2183` is NOT safe in NMI

Main-thread code sometimes writes multi-byte sequences via
`$2180` after setting an address with `$2181-$2183`. **If NMI fires
mid-sequence and any code in the NMI path touches those ports**, the
address pointer corrupts silently and the main thread resumes writing
garbage to a wrong location.

The lib's NMI handler in `templates/crt0.asm` never touches
`$2180-$2183`. If you write a custom `nmiSet()` callback, do not use
any function that goes through these ports. This is documented in
`KNOWN_LIMITATIONS.md` and called out inline in
`templates/crt0.asm:798-802`.

## Cycle cost

DMA is the fastest bulk transfer the SNES offers. Approximate cost
per transfer:

- Setup overhead: ~12 cycles (writing `DMAPn`, `BBADn`, `A1Tn`/`A1Bn`,
  `DASn`).
- Per-byte transferred: ~8 master cycles (mode 1 / mode 0).
- MDMAEN trigger: ~8 cycles.

A 1 KB DMA: 12 + 1024 × 8 + 8 ≈ 8,212 cycles, ~0.6 % of a 60 Hz frame.

A 4 KB DMA: 12 + 4096 × 8 + 8 ≈ 32,800 cycles, ~2.4 % of a 60 Hz frame
(and right at the VBlank budget — only works during force blank or with
careful timing).

The lib's NMI handler reports its DMA cost as part of its overall
budget; the perf fix in commit `6d438e6` removed an unnecessary 544-byte
OAM DMA per frame for sprite-idle ROMs (~4,300 cycles regained, ~12 %
of VBlank budget).

## See also

- `lib/include/snes/dma.h` — full API reference (function signatures,
  parameter docs).
- `lib/source/dma.asm` — the assembly implementation (390 LOC).
- [HDMA tutorial](hdma.md) — the per-scanline cousin; required reading
  for any per-scanline effect work.
- [Mode 7 tutorial](mode7.md) — the interleaved-VRAM use case for
  `dmaCopyVramMode7`.
- [Scrolling tutorial](scrolling.md) — tilemap streaming, the common
  per-frame DMA payload.
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) —
  VRAM-during-active-display silent drop (🔴), VBlank DMA budget (🔴),
  bank `$00` overflow (🟢 caught at link time), WRAM data port NMI
  trap (🔴).
- [`.claude/rules/bank0_budget.md`](../../.claude/rules/bank0_budget.md)
  — the bank `$00` ratchet that bites SUPERFREE spillover.
- [`.claude/rules/nmi_audit.md`](../../.claude/rules/nmi_audit.md) —
  what the NMI handler does with DMA channels 0 and 7 every frame.
