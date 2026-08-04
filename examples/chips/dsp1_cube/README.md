# DSP-1 Cube

![Screenshot](dsp1_cube.png)

A wireframe cube's 8 corners tumbling in 3D — with every rotation computed by
the **DSP-1 coprocessor** (NEC µPD77C25), not the 65816. Each frame the CPU
hands the DSP-1 a rotation matrix and 8 model-space points; the DSP-1 rotates
them and hands back world-space coordinates, which are drawn as 8 sprites via
orthographic projection.

This is the SDK's first DSP-1 example, and the third leg of the
enhancement-chip family alongside SA-1 and Super FX.

## SNES Concepts

- **DSP-1 command interface** — the CPU drives the coprocessor over two
  registers (data + status) with an RQM handshake; see `snes/dsp1.h`
- **Offloaded 3D math** — `dsp1Attitude()` builds a rotation matrix on the DSP,
  `dsp1Objective()` transforms each vertex through it, per frame
- **Orthographic projection** — the rotated x'/y' map straight to screen; the
  DSP-1 does no rasterisation, so the CPU just places the 8 corner sprites
- **`dsp1Init()`** — issues the `$80` resync handshake so the chip starts in a
  known command-wait state

## Firmware requirement

The DSP-1 runs Sony's mask-ROM firmware. Emulators that low-level-emulate the
chip (including **luna**) need that firmware supplied — it is copyrighted and
**not shipped with the SDK**:

```bash
# put your own dump here, then it persists
cp dsp1b.rom ~/.config/luna/firmware/dsp1b.rom
# or per-run:  luna run --dsp1-rom /path/to/dsp1b.rom dsp1_cube.sfc
```

Without it, the ROM still boots but the coprocessor stays inert (the cube won't
move). High-level-emulation emulators (snes9x) run it firmware-free.

## How to Build

```bash
cd examples/chips/dsp1_cube
make
```

Run `dsp1_cube.sfc` in an emulator with the DSP-1 firmware available.

## Modules Used

`console`, `dma`, `sprite`, `dsp1`
