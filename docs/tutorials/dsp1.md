# DSP-1 Coprocessor Tutorial {#tutorial_dsp1}

The DSP-1 (a NEC µPD77C25 running Sony's fixed firmware) is the SNES's
math coprocessor — the chip behind Pilotwings' and Super Mario Kart's
pseudo-3D. Unlike the SA-1 and Super FX you do not program it: you
invoke its built-in commands (matrix, vector, projection, trig) over a
two-register port, and OpenSNES wraps that port as ordinary C calls.
This tutorial covers the mental model, the shipped API, and the
projection pipeline the `dsp1_cube` example uses.

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
| `dsp1Present()` | `$00` KAT | — → 1/0 | probe the chip (bounded, never hangs) |
| `dsp1Multiply(a,b)` | `$00` | 2 → 1 | 1.15 product |
| `dsp1Triangle(a,r)` | `$04` | 2 → 2 | r·sin, r·cos |
| `dsp1Rotate(a,x,y)` | `$0C` | 3 → 2 | 2D rotate |
| `dsp1Attitude(s,az,ay,ax)` | `$01` | 4 → 0 | build the rotation matrix |
| `dsp1Objective(x,y,z)` | `$0D` | 3 → 3 | transform a point by the matrix |
| `dsp1Parameter(…)` | `$02` | 7 → 4 | set up the projection plane |
| `dsp1Project(x,y,z)` | `$06` | 3 → 3 | world point → screen H, V + scale M |
| `dsp1Distance(x,y,z)` | `$28` | 3 → 1 | true 3D length (hardware sqrt) |
| `dsp1Range(x,y,z,r)` | `$18` | 4 → 1 | sphere test: ≤0 = inside |

Multi-word results land in the globals `dsp1_o0`/`dsp1_o1`/`dsp1_o2`
(`dsp1_o3` for Parameter); single-word commands return their value.

## The 3D pipeline (what dsp1_cube does)

```c
dsp1Init();
if (dsp1Present()) {
    /* once: camera at the origin looking along +Y (azs = 0x4000).
     * Effective focal length ≈ lfe + les. */
    dsp1Parameter(0, 0, 0, 96, 256, 0, 0x4000);
}

/* per frame */
dsp1Attitude(0x7FFF, az, ay, ax);       /* orientation, scale 1.0 */
for (each vertex) {
    dsp1Objective(vx, vy, vz);          /* model -> world (rotated)  */
    wx = dsp1_o0;  wy = dsp1_o1 + 400;  wz = dsp1_o2;  /* +Y = depth */
    dsp1Project(wx, wy, wz);            /* world -> screen           */
    sx = 124 - dsp1_o0;                 /* H is mirrored             */
    sy = 108 - dsp1_o1;                 /* V is up-positive          */
}
```

Conventions in that snippet were characterised empirically on luna
(DSP-1B firmware, 2026-09) — the references do not document them, so
treat them as measured behaviour, not gospel:

- With `azs = 0x4000` (90°), the projection looks along **+Y**: X is
  across the screen, Z is up. The all-zero Parameter setup is
  **degenerate** — every point projects to (0,0). If your scene
  collapses to one dot, this is why.
- The projected offset behaves like `(lfe + les) · x / y` — treat the
  two distances together as the FOV knob.
- H comes back mirrored (negative for +X) and V up-positive; subtract
  both from your screen centre, as SNES Y grows downward.
- `dsp1Project` also returns **M** (`dsp1_o2`), a depth scale — the
  natural driver for sizing sprites with distance.

Copy those two `dsp1Parameter` values as your starting point and tune
from there; `Parameter`'s seven scalars are only partially understood
(the third and fourth output words are still unconfirmed — see
`.claude/notes/tech/dsp1_reference.md` for the open questions).

## Distance and Range: the game-logic commands

You do not need 3D graphics to profit from the chip:

```c
u16 d = dsp1Distance(dx, dy, dz);     /* true length, hardware sqrt */
if (dsp1Range(dx, dy, dz, radius) <= 0) {  /* inside the sphere?    */
    /* proximity trigger, LOD switch, homing acquisition ... */
}
```

Both are single calls where the 65816 would need three multiplies, two
adds and (for Distance) a software square root.

## Gotchas

### 🟡 Firmware needed by low-level emulators

luna emulates the DSP-1 at low level and needs Sony's `dsp1b.rom`
(copyrighted, not shipped). Install it once —
`cp dsp1b.rom ~/.config/luna/firmware/` or
`luna state --dsp1-rom <path> <rom>` — and it persists. Without it the
ROM boots but the chip stays inert; that is why `dsp1Present()` exists
and why the DSP-1 tests are firmware-gated (they SKIP, not fail, in
CI). Target revision is DSP-1B, the bug-fixed one and the emulator
default.

### 🟡 Not NMI-callback-safe

A command is a multi-byte DR transaction. If an `nmiSet()` callback
issues a DSP-1 call while the main loop is mid-transaction, both are
corrupted — same class as the `fixMul()` restriction in
`KNOWN_LIMITATIONS.md`. Keep all DSP-1 work in the main loop.

### 🟡 Throughput is per-byte

Every word costs two RQM-gated byte moves. The cube's 8 vertices ×
(Objective + Project) ≈ 100 word transfers per frame — fine at 60 fps,
but budget before scaling up: reuse the Attitude matrix, only re-run
Parameter when the camera moves, and batch what you can.

## See also

- [`examples/chips/dsp1_cube`](../../examples/chips/dsp1_cube/README.md) — the worked example
- `snes/dsp1.h` — full API reference with per-slot types
- `.claude/notes/tech/dsp1_reference.md` — command reference + open questions
- [SA-1 tutorial](sa1.md), [Super FX tutorial](superfx.md) — the other chips
