# HDMA Wave Table

![Screenshot](screenshot.png)

Builds a raw HDMA table by hand in C and animates it krom-style: the table is
written once, and each VBlank only the table **start pointer** advances by one
entry — the ripple pattern flows up the screen without rewriting a single
table byte. This is the classic per-scanline effect plain DMA cannot do: a
different BG1 horizontal scroll value on every line.

C port of "SNES Wave HDMA Demo" by krom (Peter Lemon),
[github.com/PeterLemon/SNES](https://github.com/PeterLemon/SNES)
(`PPU/HDMA/WaveHDMA`), reproduced on the `snes/hdma.h` API in the same
configuration as the original: **BG Mode 3, a full-screen 256-color image**
(~57 KB of unique 8bpp tiles split across two ROM banks, tilemap above them).
The art is original (procedurally generated water caustics, `res/water.bmp` —
no krom assets).

The companion example [`hdma_wave`](../hdma_wave) shows the same effect
through the library's high-level engine (`hdmaWaveH`/`hdmaWaveUpdate`); this
one is the low-level counterpart that teaches the HDMA **table format**.

## Fidelity to the original (measured, not assumed)

Register-level — krom's writes vs what this ROM's generated code does:

| Register | krom (ASM) | this example (via the C API) |
|---|---|---|
| `$4300` DMAP0 | `%00000010` | `hdmaSetup` mode = `HDMA_MODE_1REG_2X` (0x02) |
| `$4301` BBAD0 | `$0D` (BG1HOFS) | `HDMA_DEST_BG1HOFS` (0x0D) |
| `$4302-3` A1T0 | table start, **+3 bytes/frame** | `hdmaSetup(…, wave_table + phase*3)` per VBlank |
| `$4304` A1B0 | `$00` | far pointer's bank byte (table in bank-$00 RAM) |
| `$420C` HDMAEN | `%1` once | `hdmaEnable(1 << HDMA_CHANNEL_0)` once |
| `$2105` BGMODE | `$0B` (mode 3 + BG3-prio bit, no-op in mode 3) | `setMode(BG_MODE3, 0)` → mode 3 |
| `$2107` BG1SC | `$FC` (word $FC00 → mirrors $7C00) | `bgSetMapPtr(0, 0x7C00, SC_32x32)` |

One deliberate difference: krom rewrites only `A1T0L` each frame; this
example re-runs `hdmaSetup` (same five values) — semantically identical.

Behavioral — proven EXACT (luna v1.9.0 `dma.channels[]` + displacement-field
analysis on `luna frames` sequences of both ROMs):

| Proof | Result |
|---|---|
| HDMA table | **byte-identical**: krom's 896 entries extracted verbatim (`res/wavetable.bin`) |
| This ROM's rendering | displacement field == table prediction, **residual 0** over 1400 line-measurements |
| This ROM's cadence | exactly one +3-byte A1T0 step per frame, at VBlank (register-level, `dma.channels[0].a_addr`) |
| Reference's rendering | every frame pair fits the same table with **residual 0** |
| Reference's cadence in luna | irregular 2,1,1 entries/frame — a luna `$4210` polling emulation issue (luna#107), not a demo or port defect; on hardware both ROMs advance +3/frame |

## SNES Concepts

- HDMA table format: `count` byte + register payload per entry, `0x00` terminator
- `HDMA_MODE_1REG_2X`: one register written twice per line — the shape the
  16-bit scroll registers expect
- Animation by start-pointer repoint: immutable table, no tearing, ~zero CPU
- The 224+period entry layout: ≥224 valid entries from any start phase
- Mode 3 (8bpp): a >32 KB tileset split across two ROM sections, loaded with
  two `dmaCopyVram` calls (post-A6 far pointers carry the bank)

## How to Build

```bash
make
```

## Modules Used

console, dma, background, hdma
