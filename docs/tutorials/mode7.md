# Mode 7 Tutorial {#tutorial_mode7}

This tutorial covers SNES Mode 7: what the affine background mode actually
is, why its VRAM format is interleaved, the matrix maths the lib hides
behind `mode7SetAngle`/`mode7SetScale`, and how the F-Zero / Pilotwings
"perspective ground plane" pattern is built by combining Mode 7 with HDMA.
It assumes you have read the [Graphics](graphics.md) tutorial and (for the
perspective section) the [HDMA tutorial](hdma.md).

## What Mode 7 actually is

The SNES PPU has eight BG modes, numbered 0–7. The first seven (Mode 0
through Mode 6) are variations on tilemap-based rendering with different
colour-depth / layer-count tradeoffs. Mode 7 is the odd one out:

- **One BG layer only** (BG1). Mode 7 turns off BG2/3/4 entirely.
- **Tilemap is fixed at 128 × 128 entries** (one byte per entry, indexes
  into a tileset of up to 256 tiles).
- **Tiles are 8 × 8 pixels at 8 bpp** (256-colour, one byte per pixel).
- **Per-frame affine transformation**: a 2 × 2 matrix is applied to the
  whole BG layer, allowing **rotation, scaling, and shearing**.

This is the mode F-Zero, Super Mario Kart, Pilotwings, and Final Fantasy
VI's airship use. The single-layer / 128 × 128 / 8 bpp restrictions are
the cost of the affine transform — which the SNES PPU does in hardware
once per frame, not per scanline.

(Per-scanline matrix changes — the F-Zero "ground recedes into the
distance" effect — are *not* a Mode 7 feature; they are HDMA writing
new matrix values every scanline. See the perspective section below.)

## The matrix and the registers

The Mode 7 affine transformation maps each *screen* pixel `(x, y)` to a
*tilemap* coordinate `(X, Y)` through a 2 × 2 matrix and two centre /
scroll offsets:

```
X = A × (x - cx) + B × (y - cy) + sx + cx
Y = C × (x - cx) + D × (y - cy) + sy + cy
```

The registers live at `$211B`–`$2120`:

| Register | Name | Purpose |
|---|---|---|
| `$211B` (write twice) | M7A | Matrix top-left, signed 1.7.8 fixed-point |
| `$211C` (write twice) | M7B | Matrix top-right |
| `$211D` (write twice) | M7C | Matrix bottom-left |
| `$211E` (write twice) | M7D | Matrix bottom-right |
| `$211F` (write twice) | M7X | Centre X (signed 13-bit) — `cx` in the formula above |
| `$2120` (write twice) | M7Y | Centre Y (signed 13-bit) — `cy` |
| `$210D` (BG1HOFS, write twice) | scroll X | `sx` |
| `$210E` (BG1VOFS, write twice) | scroll Y | `sy` |
| `$211A` | M7SEL | Flip + out-of-bounds behaviour |

All of `$211B`–`$2120` are **double-write** registers: write the low
byte first, then the high byte, to the same address. The 65816's STA
to a 16-bit register doesn't do this for you — the lib's helpers
sequence the writes correctly.

## The interleaved VRAM format

Mode 7's biggest gotcha lives in the VRAM layout. To save bandwidth, the
PPU stores the **tilemap in the low byte of each VRAM word, and the tile
pixel data in the high byte of each VRAM word** — both occupying the
same address range `$0000`–`$3FFF`.

This means a single VRAM word at address `N` contains:

```
high byte (bits 8-15): tile pixel data byte N
low  byte (bits 0-7) : tilemap entry N
```

Implications:

1. **`dmaCopyVram` cannot load Mode 7 data correctly** in one call. It
   writes the same byte to both halves of the VRAM word. You need two
   passes: one writing only the low byte (REG_VMAIN bit 7 = 0, write to
   `$2118`) and one writing only the high byte (write to `$2119`).
2. **Tilemap and tiles share the address range**, so the 128 × 128
   tilemap occupies the entire `$0000`–`$3FFF` low-byte space. There
   is no separate tile/map base register for Mode 7 (BG12NBA / BG34NBA
   are ignored).
3. **Each tile is 64 bytes (not 32)** because 8 bpp means 1 byte per
   pixel. 256 tiles × 64 bytes = 16 KB of pixel data — exactly half of
   the `$0000`–`$3FFF` range.

The lib does not currently provide a `mode7LoadGraphics()` that hides
this. Both shipped examples use an assembly helper (typically named
`asm_loadMode7Data` or `asm_loadSkyData`) to do the interleaved DMA.
Treat the asset-loading step as "write a small assembly routine, copy
the pattern from the example".

## Setup pattern

The minimum sequence to bring up a rotating Mode 7 plane:

```c
#include <snes.h>

extern void asm_loadMode7Data(void);   /* user-supplied, see examples */

int main(void) {
    u8 angle = 0;
    u16 zoom = 0x0100;   /* 1.0 in 8.8 fixed-point */

    consoleInit();

    /* Force blank for the bulk VRAM writes */
    setScreenOff();

    asm_loadMode7Data();   /* writes interleaved tile data + map + palette */

    /* Switch to Mode 7 + initialise the affine transform */
    setMode(BG_MODE7, 0);
    mode7Init();                        /* identity matrix, centre (128, 128) */
    mode7SetScale(zoom, zoom);          /* set scale before angle */
    mode7SetAngle(angle);               /* writes M7A-M7D from sin/cos × scale */

    /* Mode 7 only supports BG1 */
    setMainScreen(TM_BG1);
    setScreenOn();

    while (1) {
        WaitForVBlank();
        angle++;                /* one degree every frame, wraps at 256 */
        mode7SetAngle(angle);   /* recompute M7A-M7D */
    }
}
```

Three things to remember:

1. **`mode7Init()` does NOT set `BG_MODE7`.** It only zeroes the affine
   registers and the centre / scroll offsets. The `setMode(BG_MODE7, 0)`
   call is your responsibility — `mode7.h:54-59` says so explicitly.
2. **Set scale before angle**, not after. `mode7SetAngle()` reads the
   current scale and combines it with `sin`/`cos` to compute the matrix.
   Calling them in the wrong order writes a stale matrix that gets
   overwritten on the next angle change anyway, but produces a
   one-frame visual glitch.
3. **`setMainScreen(TM_BG1)`**, never any other layer. Mode 7 disables
   BG2/3/4 in hardware; enabling them via TM has no effect.

## Coordinate system

The default after `mode7Init()`:

- **Scale** = 1.0 (each screen pixel maps to one tilemap pixel).
- **Centre** = (128, 128) (the centre of the 128 × 128 tilemap is the
  rotation pivot).
- **Scroll** = (0, 0) (the tilemap origin is at the screen origin).

In this state, screen `(0, 0)` shows tilemap pixel `(0, 0)`, screen
`(128, 112)` shows tilemap pixel `(128, 112)`, and so on. Pure
identity.

To **rotate around the screen centre**, set both centre and scroll so
that the rotation pivot lines up with the visible region's middle. The
helper `mode7SetPivot(x, y)` does this in screen coordinates: pass the
screen pixel you want to be the rotation centre, and the lib sets `cx`,
`cy`, `sx`, `sy` appropriately.

To **zoom in**, decrease the scale below 1.0 (`0x0080` for 0.5 = 2× zoom
in). To **zoom out**, increase scale above 1.0 (`0x0200` = 0.5×, the
tilemap looks half-size and you see twice as much). Counter-intuitive at
first, but the formula tells you why: a smaller `A`/`D` means each
screen pixel covers more tilemap pixels.

## Out-of-bounds behaviour

What happens at screen pixels that map outside the 128 × 128 tilemap?
Three options, controlled by the high two bits of M7SEL (`$211A`),
exposed via `mode7SetSettings()`:

| Constant | Behaviour |
|---|---|
| `MODE7_WRAP` (`0x00`) | Tilemap wraps. The 128 × 128 tile pattern repeats infinitely. Default. |
| `MODE7_TRANSPARENT` (`0x80`) | Out-of-bounds is transparent — show backdrop colour or a layer below (in modes that combine layers, but Mode 7 has only BG1, so this means backdrop). |
| `MODE7_TILE0` (`0xC0`) | Out-of-bounds always shows tile 0. Useful when the tileset's tile 0 is a "border" or "void" graphic. |

For a flying-game where the player can fly far in any direction, `WRAP`
gives an endless ground. For a fixed-arena game (a Mode 7 chess board,
say), `TRANSPARENT` or `TILE0` keeps the world bounded.

## The lib helpers and what they do

| Function | Purpose |
|---|---|
| `mode7Init()` | Zero affine registers, centre (128, 128), identity matrix. **Does not** set `BG_MODE7`. |
| `mode7SetScale(sx, sy)` | Set X/Y scale in 8.8 fixed-point (`0x0100` = 1.0). Stored; applied on next `mode7SetAngle`. |
| `mode7SetAngle(angle)` | 0–255 angle (full circle wraps at 256). Looks up sin/cos from a table, multiplies by current scale via the hardware multiplier, writes M7A–M7D. |
| `mode7SetCenter(x, y)` | Set the rotation centre `(cx, cy)` in tilemap coordinates (signed 13-bit). |
| `mode7SetScroll(x, y)` | Set the scroll offsets `(sx, sy)` in tilemap coordinates (signed 13-bit). |
| `mode7SetPivot(x, y)` | High-level: set the rotation centre by *screen* coordinates (0–255). The lib computes `cx`/`cy`/`sx`/`sy`. |
| `mode7Rotate(degrees)` | Convenience: take 0–359 degrees and convert to the 0–255 internal angle. |
| `mode7Transform(degrees, scalePercent)` | Combined rotate + scale: `100` = 1.0, `50` = 2× zoom in, `200` = 0.5× zoom out. |
| `mode7SetMatrix(a, b, c, d)` | Direct matrix control. Bypasses the angle/scale system. For shears, non-uniform scales, or arbitrary affine effects. |
| `mode7SetSettings(M7SEL_value)` | Flip + out-of-bounds behaviour (the constants above). |

## Worked patterns (the two shipped examples)

### Basic rotation + scaling — `examples/graphics/backgrounds/mode7`

The "hello-world" of Mode 7. `mode7SetAngle()` and `mode7SetScale()`
update the matrix every frame in response to D-pad input — A/B rotate,
UP/DOWN zoom. Read the example to see the exact init sequence
(consoleInit, setScreenOff, asm_loadMode7Data, setMode(BG_MODE7, 0),
mode7Init, mode7SetScale, mode7SetAngle, setMainScreen(TM_BG1),
setScreenOn) and the per-frame update path. ~100 lines of C; the entire
demo fits on one screen.

### F-Zero perspective via HDMA — `examples/graphics/backgrounds/mode7_perspective`

The trick that gives Mode 7 its reputation. The 2 × 2 matrix is *constant
per frame* in plain Mode 7, which means the BG plane stays "flat" — no
perspective. To create the F-Zero "ground recedes into the distance"
effect, you write **different matrix values per scanline** using HDMA on
the M7A and M7D registers.

The example is a 4-channel HDMA orchestration:

- **Channel A** writes BGMODE at the split scanline (~96), switching
  from Mode 3 (sky on top) to Mode 7 (ground on bottom).
- **Channel B** writes TM (main screen enable), switching from BG2
  (sky) to BG1 (ground) at the split.
- **Channel C** writes per-scanline M7A values — wide near the camera,
  narrow far from it.
- **Channel D** writes per-scanline M7D values — same compression
  curve.

Read the example's `asm_setupHdmaPerspective` for the table-build
algorithm. The math is straightforward: for scanline `s` at distance
`d` from the horizon, the matrix factor is `1.0 / d` (inverse depth).
What's hard is the channel bookkeeping — get the timing wrong and you
see the wrong mode for one scanline, or the perspective curve "jumps".

This is also a textbook case for the [HDMA tutorial](hdma.md)'s
gotchas: the example uses HDMA channels 1–4, leaves channel 0 for
`dmaCopyVram` and channel 7 for the OAM DMA. Run any other split-mode
display through it; the scaffolding is the same.

## Gotchas

### 🔴 Single BG layer

Mode 7 disables BG2, BG3, BG4 *in hardware*. `setMainScreen(TM_BG1 |
TM_BG2)` does nothing useful; only BG1 renders. If you want a sky on
top of a Mode 7 ground, you HDMA the BGMODE register mid-frame
(see the perspective example). There is no "Mode 7 + a normal
tilemap" mode.

OBJ (sprites) work as normal in Mode 7 — they are not BG layers and are
not affected by the mode switch. You can put sprites on top of a Mode 7
plane.

### 🔴 Interleaved VRAM, no `mode7LoadGraphics` helper today

`dmaCopyVram()` writes to both bytes of each VRAM word; for Mode 7 you
want only the low byte (tilemap) or only the high byte (tile data) per
pass. The lib does not currently ship a function that hides this — both
examples use an assembly helper. Pattern:

```asm
; Load Mode 7 tile pixel data: high-byte writes
sep #$20
lda.b #$80          ; VMAIN: increment after high byte write
sta.l $2115
rep #$20
stz.l $2116         ; VRAM address = $0000 (the start)
; ... DMA setup, source = tile data, dest = $2119 (VMDATAH)
sta.l $420B         ; trigger MDMA channel 0

; Load tilemap: low-byte writes  
stz.l $2116         ; VRAM address = $0000 again
; ... DMA setup, source = tilemap, dest = $2118 (VMDATAL)
sta.l $420B
```

Treat the assembly helper as standard furniture for any Mode 7 project.

### 🟠 Double-write register sequencing

M7A–M7D, M7X, M7Y, BG1HOFS, BG1VOFS are all double-write registers —
the SNES PPU expects the low byte then the high byte, and remembers
the previous "low byte" between writes. If you write to *another*
double-write register in between, the next high-byte write goes to the
wrong place.

The lib's helpers do this correctly. If you write to M7A–M7D directly
(via `mode7SetMatrix` or by hand), keep all four matrix writes
together — don't intersperse with other PPU writes.

### 🟡 Set scale BEFORE angle

`mode7SetAngle` reads the current scale to compute the matrix. Reverse
order writes the matrix using stale scale values for one frame; the
*next* `mode7SetAngle` call corrects it. Cosmetic, but visible as a
one-frame "blip" when you change scale and angle in the same frame.

### 🟡 Centre default is the tilemap centre, not the screen centre

`mode7Init` sets `cx = cy = 128`, which is the centre of the 128 × 128
*tilemap*, not the centre of the 256 × 224 *screen*. To rotate around
the screen centre, use `mode7SetPivot(128, 112)` (which positions the
visible region's centre at the rotation pivot).

This trips people up: "my plane rotates correctly but isn't centred on
screen". The pivot helper is the fix.

### 🟡 Out-of-bounds with `MODE7_TRANSPARENT` shows the backdrop, not a layer

Mode 7 has no other BG to "see through to". `MODE7_TRANSPARENT` shows
the backdrop colour (CGRAM[0]) where the tilemap doesn't extend. If you
want a sky behind the Mode 7 plane, you must HDMA the BGMODE register
to switch to a different mode for the sky region of the screen — see
the perspective example.

## Cycle cost

Mode 7's affine transformation is computed in PPU hardware, not on the
CPU, so the *display* of a Mode 7 plane is free. The CPU cost lives in
the matrix updates:

- `mode7SetAngle()` — sin/cos lookup + four hardware multiplies + four
  PPU writes ≈ 60–80 cycles per call.
- `mode7Transform()` — same, with the degrees → angle conversion ≈
  100–120 cycles.
- A direct `mode7SetMatrix()` — just the four PPU writes ≈ 30–40
  cycles.

Per-frame matrix updates are cheap (well under 0.1 % of the frame).
Per-scanline updates via HDMA are more expensive — see the
[HDMA tutorial's cycle-cost section](hdma.md). The 4-channel
perspective example consumes ~6 % of CPU during active display.

## See also

- `lib/include/snes/mode7.h` — full API reference.
- `lib/source/mode7.asm` — implementation (matrix multiply, sin/cos
  table).
- [`examples/graphics/backgrounds/mode7`](../../examples/graphics/backgrounds/mode7/README.md) — basic rotation + scaling demo.
- [`examples/graphics/backgrounds/mode7_perspective`](../../examples/graphics/backgrounds/mode7_perspective/README.md) — F-Zero-style perspective via 4-channel HDMA.
- [HDMA tutorial](hdma.md) — required reading for the perspective
  pattern.
- [Graphics tutorial](graphics.md) — companion read for the regular
  BG modes (0–6).
- [`KNOWN_LIMITATIONS.md`](../../KNOWN_LIMITATIONS.md) — VBlank DMA
  budget, bank `$00` overflow, and the runtime traps that bite Mode 7
  projects with large 8 bpp tilesets.
