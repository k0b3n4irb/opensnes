# HDMA Indirect Gradient

![Screenshot](screenshot.png)

A 32-band red gradient rendered entirely by an **indirect** HDMA channel on
the naked backdrop — no BG layers, no tiles, no assets, zero per-frame CPU.
The HDMA table carries `[count][pointer]` entries; the PPU fetches each
band's 4-byte payload (`[CGADD][CGADD][color_lo][color_hi]`) through the
pointer and rewrites CGRAM color 0 every 7 scanlines.

C port of "SNES Red Space Indirect HDMA Demo" by krom (Peter Lemon),
[github.com/PeterLemon/SNES](https://github.com/PeterLemon/SNES)
(`PPU/HDMA/RedSpaceIndirectHDMA`), on the `snes/hdma.h` API — the permanent
consumer of `hdmaSetupIndirect()` (the register plain `hdmaSetup` never
programs, `$43x7`, is the whole point of this mode).

The companion [`gradient_colors`](../gradient_colors) draws the same class
of effect with a **direct** table (data inline); here the payload lives in
a separate WRAM array the CPU can edit freely — the layout used for dynamic
per-scanline palettes (the HiColor building block).

## Fidelity to the original (measured, not assumed)

Register-level — krom's writes vs this ROM's generated code:

| Register | krom (ASM) | this example (via the C API) |
|---|---|---|
| `$4300` DMAP0 | `%01000011` (indirect \| 2REG_2X) | `hdmaSetupIndirect` ORs `HDMA_INDIRECT` into `HDMA_MODE_2REG_2X` |
| `$4301` BBAD0 | `$21` (CGADD) | `HDMA_DEST_CGADD` (0x21) |
| `$4302-3` A1T0 | table address (static) | `itable` far pointer, set once |
| `$4304` A1B0 | `$00` | table pointer's bank byte ($00 WRAM) |
| `$4307` DASB0 | `$00` | `dataBank` param = `(u8)((u32)(void*)band_data >> 16)` |
| `$420C` HDMAEN | `%1` once | `hdmaEnable(1 << HDMA_CHANNEL_0)` once |
| `$212C` TM | `0` (backdrop only) | `setMainScreen(0)` |

Behavioral — central-column color profile of both ROMs in luna:

| Measure | krom | this example |
|---|---|---|
| Red ramp | 255 → 0 | 255 → 0 |
| Green / blue | 0 / 0 | 0 / 0 |
| Bands | 32 steps, 7-line median width | 32 steps, 7-line median width |
| **Max per-line profile difference** | — | **0** (pixel-exact) |

## SNES Concepts

- Indirect HDMA (DMAP bit 6): the table holds pointers; `$43x7` selects the
  data bank — `hdmaSetupIndirect()` programs both
- Non-repeat count: `count=7` fetches the payload once and holds 7 lines
  (a repeat count `$87` would fetch fresh data every line — HiColor)
- `HDMA_MODE_2REG_2X` to CGADD streams `$2121,$2121,$2122,$2122`: each block
  rewrites CGRAM color 0, i.e. the backdrop
- Bank of a C array for `$43x7`: `(u8)((u32)(void *)arr >> 16)`

## How to Build

```bash
make
```

## Modules Used

console, dma, hdma
