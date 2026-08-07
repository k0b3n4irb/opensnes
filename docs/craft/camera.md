# Following the player: the camera {#craft_camera}

@ref craft_backgrounds composed the *layers*; this guide moves the *window* over
them. A camera is how the visible 256×224 screen tracks the player through a
world larger than itself — and on the SNES there is no camera object to
configure, only scroll registers you write every frame. The universal theory of
good cameras is written down beautifully already (see the Go-deeper box); this
guide is how to land it on this hardware, with this SDK. For the registers
themselves, see @ref snes_graphics_guide.

## The camera *is* your scroll values

There is no viewport primitive. Moving the camera right means increasing BG1's
horizontal scroll, which you push with `bgSetScroll` and the PPU latches at
VBlank. Everything below is just *what number to feed it each frame*.

Two consequences fall straight out of the earlier guides:

- **Parallax is the camera, scaled.** Your midground and sky are the same camera
  position multiplied by a fraction — see @ref craft_backgrounds. One camera
  drives every layer; you never track them separately.
- **It is a PPU write, so it happens in VBlank.** Compute the new camera during
  active display; the scroll latch lands at VBlank (@ref craft_frame_budget).
  The SDK's NMI scroll-sync does the latching for you.

## Don't glue the camera to the player — deadzone it

The beginner camera sets scroll to the player's position. It works, and it is
subtly awful: every idle bob and half-pixel shoves the whole screen, and the
eye never rests. The fix is a **deadzone** (camera window): a box in the middle
of the screen the player moves *inside* freely; the camera only scrolls when
they reach its edge, and then only enough to keep them in the box.

```
   screen
 ┌───────────────┐     player roams the inner box for free;
 │   ┌───────┐   │     the camera holds still. Push the edge
 │   │  ▶    │   │     and the camera scrolls just enough to
 │   └───────┘   │     keep the box around the player.
 └───────────────┘
```

A wide horizontal deadzone with almost no vertical one is the platformer
default: free side-to-side wander, tight vertical tracking.

## Lead the action — look-ahead

Bias the camera *ahead* of the player in the direction they face or move, so
they see what they are running into rather than sitting dead-centre. The trick
is to move the target smoothly — ease the camera toward the offset over several
frames rather than snapping — or a facing-flip turns into a lurch. This is the
one place a little per-frame interpolation earns its cost.

## Vertical wants different rules

Vertical camera motion is where cheap cameras get sickening: follow the
player's Y exactly and every jump heaves the screen. Common fixes, in order of
reach:

- **Snap on ground only.** Track Y while grounded; during a jump, hold the
  camera and let the player rise and fall within the frame.
- **Platform bands.** Move the vertical camera in discrete steps when the player
  settles on a new platform, not continuously.

@ref examples_games_likemario is a full platformer to read for how camera,
scroll and streaming fit together.

## Clamp to the world — room-locking

A camera that follows past the level's edge reveals the void beyond your
tilemap. **Clamp the scroll** to the world: never below 0, never past
`level_pixels − screen_pixels` on each axis. This is also where the camera meets
the streaming map from @ref craft_tiles_to_levels — the map engine ties the
camera's position to which column it streams next, and the same clamp that stops
the view at the edge stops you streaming past the last column.

| Follow style | Feeds | See |
|--------------|-------|-----|
| Whole screen scrolls with the player | `bgSetScroll` per layer | @ref examples_scrolling_mixed_scroll |
| Layers scroll at different rates | one camera × per-layer fraction | @ref examples_scrolling_parallax_scroll |
| World wider than VRAM, streamed | camera drives column streaming | @ref examples_maps_map_scroll, @ref examples_scrolling_continuous_scroll |
| Free-scrolling 2D field | clamp both axes to bounds | @ref examples_maps_dynamic_map |

## Write it at the right moment

Camera scroll is a PPU register write, so it obeys the @ref craft_frame_budget —
do the follow math during active display, and let the scroll latch at VBlank. Latch
mid-frame by hand and you tear the image — a layer scrolled on line 100 shows
the seam. Let the NMI scroll-sync apply it and the whole screen moves as one.

> **Go deeper.** For the movement theory itself — deadzones, look-ahead,
> platform snapping, room-locking, and a taxonomy of every camera in the
> canon — read Itay Keren's
> [Scroll Back](https://docs.google.com/document/d/1iNSQIyNpVGHeak6isbP6AHdHD50gs8MNXF1GCf08efg/pub).
> Compose your layers in @ref craft_backgrounds; decide how the camera *follows*
> here; feed both from one camera value.

## The one rule

Track one camera position; derive everything — every layer's scroll, every
parallax fraction, the streaming boundary — from it. The player should feel the
world move, never the camera work. A deadzone plus an edge clamp gets you most
of the way there before you write a line of easing.

> **Sources:**
> [fullsnes](https://problemkaputt.de/fullsnes.htm) and the
> [SNESdev Wiki](https://snes.nesdev.org/wiki) (BG scroll registers, VBlank
> latch); the SDK's map engine (`lib/include/snes/map.h`) for camera-driven
> streaming.
