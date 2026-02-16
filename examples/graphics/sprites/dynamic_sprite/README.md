# Dynamic Sprite — VRAM Streaming for Animated Sprites

## What This Example Shows

How to use the **dynamic sprite system** to stream sprite tile data into VRAM every frame.
Unlike static sprites (where all frames live in VRAM permanently), dynamic sprites upload
only the current animation frame, saving VRAM space when sprites have many frames.

## Prerequisites

Read `sprites/simple_sprite` first (OAM basics), then `sprites/animated_sprite`
(frame-based animation).

## How It Works

### 1. Initialize the dynamic sprite engine

```c
oamInitDynamicSprite(0x0000, 0x1000, 0, 0, OBJ_SIZE8_L16);
```

This configures the engine:
- VRAM base: `$0000` (where streaming sprites start)
- VRAM ceiling: `$1000` (max VRAM used — prevents overwriting background data)
- Sprite size: 8x8 small / 16x16 large

### 2. Set up sprites via the oambuffer

```c
oambuffer[0].oamx = 64;
oambuffer[0].oamy = 100;
oambuffer[0].oamframeid = 0;
oambuffer[0].oamattribute = OBJ_PRIO(3);
oambuffer[0].oamrefresh = 1;
OAM_SET_GFX(0, spr16_tiles);
```

Each dynamic sprite has:
- **oamframeid**: which animation frame to display (index into the sprite sheet)
- **oamrefresh**: set to 1 when the frame changes — tells the engine to re-upload tiles
- **OAM_SET_GFX**: points to the source tile data in ROM

### 3. Draw and upload every frame

```c
oamDynamic16Draw(0);        /* Queue sprite 0 for VRAM upload */
oamVramQueueUpdate();       /* DMA all queued tiles to VRAM */
oamInitDynamicSpriteEndFrame();  /* Reset for next frame */
```

The draw/upload cycle runs every frame in the main loop. Only sprites with
`oamrefresh = 1` actually trigger a DMA transfer.

### 4. Animate by changing frame IDs

```c
frame0++;
if (frame0 >= 24) frame0 = 0;
oambuffer[0].oamframeid = frame0;
oambuffer[0].oamrefresh = 1;
```

Every 8 frames, the animation advances. Setting `oamrefresh = 1` tells the engine
to upload the new frame's tiles to VRAM on the next `oamVramQueueUpdate()` call.

## SNES Concepts

- **VRAM budget**: The SNES can only write to VRAM during VBlank (~2,280 bytes via DMA
  per frame). Dynamic sprites must fit within this budget — fewer sprites or smaller
  tiles means more headroom.
- **Static vs dynamic sprites**: Static = all frames pre-loaded in VRAM (fast but
  uses lots of VRAM). Dynamic = stream on demand (slower but supports many more frames).
- **Force blank**: This demo uses `REG_INIDISP = 0x80` during setup to allow unlimited
  VRAM writes. During gameplay, writes happen via DMA in VBlank.

## Next Steps

- `games/breakout` — Dynamic sprites in a real game context
- `games/likemario` — Dynamic sprites with scrolling backgrounds
