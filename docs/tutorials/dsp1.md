# DSP-1 Coprocessor Tutorial {#tutorial_dsp1}

The DSP-1 (a NEC µPD77C25 running Sony's fixed firmware) is the SNES's
math coprocessor — the chip behind Pilotwings' and Super Mario Kart's
pseudo-3D. Unlike the SA-1 and Super FX you do not program it: you
invoke its built-in commands (matrix, vector, projection, trig) over a
two-register port, and OpenSNES wraps that port as ordinary C calls.
This tutorial covers the mental model, the shipped API, and the
projection pipeline the `dsp1_ground` example uses.

## What the chip actually is

A fixed-function DSP on the cartridge, mapped into the address space —
not a second CPU. On the LoROM board this SDK targets, its two byte-wide
registers live at ([fullsnes, DSP
mapping](https://problemkaputt.de/fullsnes.htm#snescartdspnst010st011necupd77c25registersflagsoverview)):

| Register | Address | Role |
|---|---|---|
| DR (data) | `$30:8000` | commands in, parameters in, results out |
| SR (status) | `$30:C000` | bit 7 = RQM, the only handshake flag |

Enable it with one Makefile line — `USE_DSP1 := 1` — which sets the
ROM-header cartridge type to `$03` (ROM + DSP). The `dsp1` lib module is
linked automatically.

## The handshake, in one paragraph

Every transfer is: **poll RQM until set, then move one byte**. Commands
are a single byte; parameters and results are 16-bit words sent as two
bytes, **LSB first**. That is the whole protocol — the lib's driver
(`lib/source/dsp1.asm`) does it for you, so from C a command is just a
function call that returns when the transaction is complete. Commands
have fixed in/out word counts; the driver knows them, which is also why
you should never poke `$30:8000` yourself mid-frame: a half-finished
transaction desynchronises the chip (recover with `dsp1Init()`, which
hammers the `$80` Sync/Reset byte).

## Fixed-point: three slot types

The DSP-1 types its operands per parameter slot:

| Type | Meaning | Format |
|---|---|---|
| I | integer | signed 16-bit (coordinates, distances) |
| T | fraction | signed 1.15 — `0x7FFF` ≈ +1.0, `0x4000` = 0.5 |
| A | angle | signed 16-bit, full turn = 2^16 — `0x4000` = 90° |

These are **not** the SDK's own formats (`fixed` is 8.8, `fixSin`
angles are 8-bit). `snes/dsp1.h` ships bridge macros:
`DSP1_T_FROM_FIX(f)`, `DSP1_FIX_FROM_T(t)`, `DSP1_A_FROM_FIX8(a)`.
Mind the T range: it is −1..+1, so clamp before converting anything
that can exceed ±1.0.

## The API at a glance

| Call | DSP command | In → Out | Use for |
|---|---|---|---|
| `dsp1Init()` | `$80` ×128 | — | resync at boot / after a desync |
| `dsp1IsPresent()` | `$00` KAT | — → 1/0 | probe the chip (bounded, never hangs) |
| `dsp1Multiply(a,b)` | `$00` | 2 → 1 | 1.15 product |
| `dsp1Triangle(a,r)` | `$04` | 2 → 2 | r·sin, r·cos |
| `dsp1Rotate(a,x,y)` | `$0C` | 3 → 2 | 2D rotate |
| `dsp1Attitude(s,az,ay,ax)` | `$01` | 4 → 0 | build the rotation matrix |
| `dsp1Objective(x,y,z)` | `$0D` | 3 → 3 | transform a point by the matrix |
| `dsp1SetCamera(&cam)` | `$02` | 7 → 4 | set up the projection plane (the seven inputs are the fields of `Dsp1Camera`) |
| `dsp1Project(x,y,z)` | `$06` | 3 → 3 | world point → screen H, V + scale M |
| `dsp1Target(h,v)` | `$0E` | 2 → 2 | screen point → ground plane (pick / aim) |
| `dsp1Raster(ab,cd,vs,n)` | `$0A` | 1 → 4·n (stream) | per-scanline Mode 7 matrices into HDMA payloads |
| `dsp1Distance(x,y,z)` | `$28` | 3 → 1 | 3D length (hardware sqrt; reads one low on exact lengths) |
| `dsp1Range(x,y,z,r)` | `$18` | 4 → 1 | sphere test: `(d² − r²) >> 15`, negative = inside |

Multi-word results land in the globals `dsp1_o0`/`dsp1_o1`/`dsp1_o2`
(`dsp1_o3` for `dsp1SetCamera`); single-word commands return their value.

## Projecting points (the objects of dsp1_ground)

`dsp1Project` turns a world point into a screen position with the camera
of the last `dsp1SetCamera` — the same camera that draws the ground, so
an object projected with it stays glued to the floor. `dsp1_ground` puts
four beacons on the ground and a cube above it, whose corners
`dsp1Attitude` + `dsp1Objective` rotate first:

```c
dsp1SetCamera(&cam);                    /* the ground's camera        */
...
dsp1Attitude(0x7FFF, az, ay, 0);        /* orientation, scale 1.0     */
for (each corner) {
    dsp1Objective(vx, vy, vz);          /* model -> rotated offsets   */
    ox = dsp1_o0; oy = dsp1_o1; oz = dsp1_o2;   /* Project reuses them */
    dsp1Project(CX + ox, CY + oy, CZ + oz);     /* world -> screen    */
    if (dsp1_o2 <= 0) hide();           /* M <= 0: behind the eye     */
    sx = 128 + dsp1_o0;                 /* H: right-positive          */
    sy = 112 + dsp1_o1;                 /* V: down-positive           */
}
```

The conventions, measured on luna (DSP-1B firmware) and stated the same
way by bsnes's DSP-1 code ("H is positive rightward, but V is positive
downward", `dsp1emu.cpp`, not an arbiter):

- **H and V are relative to the screen centre, H to the right, V
  downward.** A point raised by 40 units moves up the screen (V 29 → 8),
  one moved to +X with the camera heading 0 moves right.
- **M (`dsp1_o2`) is the scale**, and its sign says which side of the eye
  the point is: positive in front (135 under the screen centre in
  `dsp1_ground`), negative behind. A point behind the eye still comes back
  with an H and a V — mirrored — so test M before drawing.
- With `aas = 0` the camera looks along **−Y** (up the map); `azs` is the
  zenith angle of the view line, `0x4000` horizontal. The all-zero
  Parameter setup is **degenerate** — every point projects to (0,0). If
  your scene collapses to one dot, this is why.
- The projected offset behaves like `(lfe + les) · x / depth` — treat the
  two distances together as the FOV knob.

Copy the two lens values of `dsp1_ground` (`lfe` 96, `les` 256) as your
starting point and tune from there. `dsp1SetCamera`'s four output words are **Vof, Vva, Cx, Cy** (official
manual §5.4.1, verified on luna): the raster of the "imaginary centre",
the horizon raster relative to it, and the ground point under that
centre. They are what the ground pipeline below runs on.

## The ground (what dsp1_ground does)

The reason the chip was on the Super Mario Kart cartridge: the **Raster**
command. Give it a camera and it streams, per scanline, the 2×2 Mode 7
matrix that projects the ground plane — perspective *and* rotation, A,
B, C and D — as an open-ended stream the CPU stops with a sentinel. The
lib wraps the whole transaction:

```c
static Dsp1Camera cam = { .z = height, .lfe = 96, .les = 256, .azs = tilt };

cam.x = x;  cam.y = y;  cam.aas = heading;   /* what moves */
dsp1SetCamera(&cam);
vof = dsp1_o0;  vva = dsp1_o1;  cx = dsp1_o2;  cy = dsp1_o3;

/* ground = rasters Vva+2 .. (Vva+1 is the singular horizon line),
 * on screen lines 112+Vof+Vva+2 .. 223                          */
dsp1Raster(&tab_ab[HDR], &tab_cd[HDR], vva + 2, lines);

/* VBlank: swap the HDMA tables and pin the centre */
hdmaSetTable(CH_AB, tab_ab);  hdmaSetTable(CH_CD, tab_cd);
mode7SetCenter(cx, cy);
mode7SetScroll(cx - 128, cy - (112 + vof));
```

`tab_ab`/`tab_cd` are `HDMA_MODE_2REG_2X` repeat blocks (M7A/M7B on one
channel, M7C/M7D on the other) whose payload the chip fills directly;
`HDR` is the header the caller lays out. Three things to know:

- **Raster numbers are down-positive and relative to the screen
  centre**, like `dsp1Project`'s V. The horizon is on
  screen line `112 + Vof + Vva`; `Vof` is where the firmware parks its
  reference line (0 when looking well down, positive when the horizon is
  near or below the centre).
- **The D values are slopes relative to the imaginary centre line**, so
  M7VOFS must put the pivot (M7X/M7Y = Cx/Cy) on that line — the
  `mode7SetScroll` above. Get this wrong and the ground shears.
- **Cost** (measured on luna, not a datasheet figure): ≈ 50 µs per raster
  (per-word RQM handshake, 4 words), so a
  126-line floor is ≈ 40 % of a frame. Super Mario Kart streams 96 lines
  per frame. Double-buffer the tables and fill the back set during active
  display, as the example does.

`dsp1Target(h, v)` is the inverse for the same plane: the ground point
under a screen pixel, in raster coordinates (Target(0, 0) = Cx/Cy).

## Distance and Range: the game-logic commands

You do not need 3D graphics to profit from the chip:

```c
u16 d = dsp1Distance(dx, dy, dz);     /* length, hardware sqrt (within 1) */
if (dsp1Range(dx, dy, dz, radius) < 0) {   /* clearly inside the sphere? */
    /* proximity trigger, LOD switch, homing acquisition ... */
}
```

Two properties measured on luna's DSP-1B firmware and pinned by the library
fixture (`testing/fixtures/libtests_dsp1`), neither stated by any hardware reference
we could find. `dsp1Distance` reads one low on exact lengths — (3, 4, 12)
gives 12 — so compare with `>=` / `<`, never `==`. `dsp1Range` returns the
squared difference **shifted right by 15**, not the raw difference: a point
less than 32768 squared-units outside the surface reads 0, the same as one
exactly on it. Choose world units so that margin is small next to the radius.

Both are single calls where the 65816 would need three multiplies, two
adds and (for Distance) a software square root.

## Gotchas

### 🟡 Firmware needed by low-level emulators

luna emulates the DSP-1 at low level and needs Sony's `dsp1b.rom`
(copyrighted, not shipped). Install it once —
`cp dsp1b.rom ~/.config/luna/firmware/` or
`luna state --dsp1-rom <path> <rom>` — and it persists. Without it the
ROM boots but the chip stays inert; that is why `dsp1IsPresent()` exists
and why the DSP-1 tests are firmware-gated (they SKIP, not fail, in
CI). Target revision is DSP-1B, the bug-fixed one and the emulator
default.

### 🟡 Not NMI-callback-safe

A command is a multi-byte DR transaction. If an `nmiSet()` callback
issues a DSP-1 call while the main loop is mid-transaction, both are
corrupted — same class as the `fixMul()` restriction in
`KNOWN_LIMITATIONS.md`. Keep all DSP-1 work in the main loop.

### 🟡 A Raster stream cannot be "resynced" — let the lib close it

`dsp1Init()`'s `$80` Sync recovers an interrupted *fixed-length* command,
not an open stream: inside a Raster stream every byte pair is swallowed
as a 16-bit word, so a wedged stream never returns to command mode until
`$8000` lands on a D slot. `dsp1Raster()` always closes its stream; do
not hand-roll `$0A` (and never `$1A`, which the DSP-1B firmware never
returns from — measured on luna).

### 🟡 Throughput is per-byte

Every word costs two RQM-gated byte moves. `dsp1_ground`'s 8 cube corners
× (Objective + Project) plus 4 beacons ≈ 120 word transfers per frame — fine at 60 fps,
but budget before scaling up: reuse the Attitude matrix, only re-run
Parameter when the camera moves, and batch what you can.

## See also

- [`examples/mode7/dsp1_ground`](../../examples/mode7/dsp1_ground/README.md) — the Raster ground and the projected objects
- `snes/dsp1.h` — full API reference with per-slot types
- [SA-1 tutorial](sa1.md), [Super FX tutorial](superfx.md) — the other chips
