# The frame budget {#craft_frame_budget}

@ref craft_planning budgets *space* — the VRAM, CGRAM and OAM your scene fills
(and `make budget` measures it). This guide budgets *time*: what one frame can
actually do at 60 Hz before the game stutters. Space is what fits on the
cartridge; time is what fits in 1/60th of a second. Both are fixed, and both
are design levers.

## Where the time goes

Every 1/60 s (NTSC) the screen draws top to bottom, then blanks briefly before
the next one:

- **Active display (~224 lines):** the PPU is reading VRAM to paint pixels.
  **You cannot write VRAM now** — writes are silently dropped. This is your
  compute window: game logic, physics, AI, building next frame's data.
- **VBlank (~38 lines):** the PPU is idle. This is your **only safe window to
  touch VRAM, CGRAM and OAM** — and it is short.

The single most common frame-timing bug follows directly: do your heavy
computation *first*, during active display, and fire the VRAM upload the
instant VBlank starts. Compute *after* `WaitForVBlank()` and the upload spills
past VBlank's end — the PPU drops it, and you get garbage or a dropped update.
(@ref examples_backgrounds_mode2 hit exactly this; its loop builds the offset
table during active display and DMAs it at VBlank start.)

## The VBlank DMA budget: ~4 KB

DMA moves roughly 1 byte per fast cycle, and VBlank is only ~38 scanlines, so
the hard ceiling is about **6 KB** per VBlank — but once the NMI handler has
read the controllers and done its own housekeeping, **~4 KB is the safe working
budget** you should plan around.

That shapes real decisions:

- **A full 32×32 tilemap is 2 KB, a 4bpp tile is 32 bytes.** You can refresh a
  screen's worth of map *or* ~120 tiles per frame, not everything at once.
- **Need to move more?** Two levers:
  - **Force blank** (`setScreenOff` / `setScreenOn`, INIDISP bit 7): the PPU
    stops fetching, so you can DMA the *whole* VRAM — at the cost of a black
    frame. Right for scene transitions and level loads; wrong for a HUD tweak.
    @ref examples_basics_panel_hud shows the trade-off deliberately: its 2 KB
    panel flush uses force blank (fine on a menu open), while its frequently
    changing HUD row uses a small in-VBlank DMA (no blank).
  - **Split across frames.** Stream a big transfer a few KB at a time over
    several frames — how @ref examples_scrolling_continuous_scroll feeds new
    tile columns as the camera moves.

> **Warning, from experience.** A "black flash at the top of the screen" on an
> update almost always means a force-blank upload that overran VBlank into the
> first scanlines. Move the write into VBlank, shrink it under the budget, or
> commit to a full force-blank frame — don't leave it straddling the boundary.

## The sprite-per-line budget: 32

Separate from *how many* sprites exist (128) is *how many the PPU can draw on
one scanline*: **32 sprites, and 34 8×8 tile-slivers**, per line. Cross either
and sprites drop out — culled from the lowest OAM priority first, and an
off-screen-left sprite still counts (a documented quirk). This is a *render*
limit, distinct from the *CPU* cost of updating many sprites.

The levers: spread action vertically so the busy lines are few; keep large
sprites sparse; and remember that updating 128 sprites' positions in C is its
own cost — @ref examples_sprites_sprite_swarm measures where per-sprite C
motion runs out of frame time (around three dozen sprites), which is why heavy
sprite math often moves off the main CPU.

## The CPU budget: ~3.58 MHz

The 65816 runs at 3.58 MHz (FastROM) or 2.68 MHz (slow), so a frame is only so
many thousand cycles. When logic outgrows it, in rough order of reach-for:

- **FastROM** — a near-free ~33% speedup for ROM-bound code (a build flag).
- **Do PPU tricks in the PPU, not the CPU.** A per-scanline gradient or wave is
  free via HDMA and ruinous in a CPU loop — see the @ref examples_hdma_hdma_helpers
  family. Per-column effects go through offset-per-tile (@ref
  examples_backgrounds_mode2), not code.
- **Offload the math.** The enhancement chips exist for exactly this: @ref
  examples_chips_sa1_starfield runs its 128-bird trig on the SA-1 at 10.74 MHz,
  and Super FX handles 3D. The same murmuration in plain C would not fit a
  frame.

## When you blow the budget

The failure is graceful, not a crash: **lag frames.** Miss a VBlank and the
game simply runs at 30 fps (or 20, or 15) instead of 60 — the classic 16-bit
"slowdown." That is a legitimate design outcome, not only a bug: plenty of
beloved SNES games drop to 30 fps in their busiest moments on purpose, and
designing *for* 30 fps buys you double the CPU and DMA per update. Decide your
target framerate up front and budget to it, rather than discovering it in the
explosion that fills the screen.

## The one rule

Space is measured with `make budget`; time is measured in frames. For both,
**plan the busiest moment, not the average** — the scene with the most sprites,
the biggest transfer, the heaviest logic. If the worst case fits the frame, the
rest is comfortable.

> **Sources:**
> [fullsnes](https://problemkaputt.de/fullsnes.htm) ·
> [Anomie's SNES timing doc](https://www.romhacking.net/documents/199/) ·
> [SNESdev Wiki — DMA / VBlank](https://snes.nesdev.org/wiki/DMA_registers) ·
> [Bumbershoot Software](https://bumbershootsoft.wordpress.com/2023/10/14/dma-and-fastrom-on-the-snes-speed-at-any-cost/)
> (DMA budget & FastROM).
