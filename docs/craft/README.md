# Game-Craft Guides {#craft}

The API tutorials teach you **how to call** the SDK. These guides teach you
**how to decide** — the seam between "I know the sprite API" and "I shipped a
small game." They are hardware-grounded design advice, not reference: for the
registers and exact hardware behaviour, see @ref snes_graphics_guide.

## The philosophy

Two ideas run through every guide here:

1. **Constraints are creative fuel.** The SNES gives you 8 background modes,
   64 KB of VRAM, 256 colours, and 32 sprites per scanline. Those are not
   walls to memorise and resent — they are the design language of the machine.
   Every classic you remember was *shaped* by them. We frame each limit as a
   lever, not a ceiling.
2. **Link the universal, own the SNES-specific.** Camera design, "game feel,"
   and why the PPU works as it does are already explained brilliantly
   elsewhere — we link that gold-standard material rather than rewrite it, and
   spend our words on the one thing nobody else covers: *how to do it on real
   SNES hardware*, with this SDK.

## The guides

- @subpage craft_planning — **Planning your SNES game.** Budget your VRAM,
  CGRAM and sprites *before* you draw a pixel; choose a background mode from
  your genre, not a spec sheet; scope a first game you can actually finish.
- @subpage craft_backgrounds — **Composing with backgrounds & layers.** How
  to use BG1/BG2/BG3 and sprites together for parallax, HUDs, foreground
  occlusion — as composition decisions, anchored to working examples.
- @subpage craft_frame_budget — **The frame budget.** What one frame can do at
  60 Hz: the VBlank DMA budget, the sprite-per-line limit, CPU time, and when
  30 fps is the right call. The *time* companion to Planning's *space*.
- @subpage craft_tiles_to_levels — **From tiles to levels.** Think in metatiles,
  attach collision and meaning to tiles, stream a level bigger than VRAM, and
  author it in Tiled instead of by hand.

*(More on the way: per-technique craft companions — camera, game-feel.)*

## Go deeper — the curated shelf

When a topic is universal, we point you at the best explanation that exists.
Read these; they are worth your time:

| Topic | Resource |
|-------|----------|
| **2D cameras** (deadzones, look-ahead, room-locking) | Itay Keren, *Scroll Back: The Theory and Practice of Cameras in Side-Scrollers* — [read](https://docs.google.com/document/d/1iNSQIyNpVGHeak6isbP6AHdHD50gs8MNXF1GCf08efg/pub) |
| **Game feel / juice** | Jonasson & Purho, *Juice It or Lose It* (GDC) — [watch](https://www.gdcvault.com/play/1016487/Juice-It-or-Lose) |
| **Why the SNES PPU works as it does** | Fabien Sanglard, *SNES: Sprites and backgrounds rendering* — [read](https://fabiensanglard.net/snes_ppus_why/) |
| **Visual hardware explainers** (modes, palettes, sprites) | Retro Game Mechanics Explained — [channel](https://www.youtube.com/c/RetroGameMechanicsExplained) |
| **Metatiles & tile-based level workflow** | nesdoug, *Metatiles* — [read](https://nesdoug.com/2018/09/05/11-metatiles/) |
| **Level-design craft** (pacing, theme, variety) | SMW Central beginners' guide — [read](https://www.smwcentral.net/?p=beginners) |
| **Pixel art within a palette** (indexed, 15+1) | 2D Will Never Die, *So You Want Your Sprites to Be 16 Colors* — [read](https://2dwillneverdie.com/tutorial/so-you-want-your-sprites-to-be-16-colors/) |

## The hardware references we cite

Every number in these guides traces to one of these; when we say "≈4 KB per
VBlank" or "32 sprites per line," this is where it comes from:

- **fullsnes** (Martin Korth) — [problemkaputt.de/fullsnes.htm](https://problemkaputt.de/fullsnes.htm)
- **Anomie's SNES docs** — [romhacking.net/documents/199](https://www.romhacking.net/documents/199/)
- **SNESdev Wiki** — [snes.nesdev.org/wiki](https://snes.nesdev.org/wiki)
- **Bumbershoot Software** (DMA/VBlank budget) — [article](https://bumbershootsoft.wordpress.com/2023/10/14/dma-and-fastrom-on-the-snes-speed-at-any-cost/)
