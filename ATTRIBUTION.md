# Attribution

OpenSNES is a fork of PVSnesLib, building upon the work of many contributors
in the SNES homebrew community. We are grateful for their pioneering efforts.

## Primary Lineage

### PVSnesLib
- **Author**: Alekmaul and contributors
- **Repository**: https://github.com/alekmaul/pvsneslib
- **License**: MIT
- **What we use**: Core library (sprites, backgrounds, DMA, HDMA, input, audio),
  tool architecture, build system patterns, example structure
- **Status**: OpenSNES is a fork with a different compiler, expanded library,
  and improved tooling

### SNES-SDK
- **Author**: Ulrich Hecht
- **Repository**: http://code.google.com/p/snes-sdk/
- **What it is**: The original SNES C SDK that PVSnesLib built upon

## Compiler Stack

### QBE (Compiler Backend)
- **Author**: Quentin Carbonneaux
- **Repository**: https://c9x.me/compile/
- **License**: MIT
- **What we use**: Compiler infrastructure with our custom w65816 backend

### cproc (C Compiler Frontend)
- **Author**: Michael Forney
- **Repository**: https://git.sr.ht/~mcf/cproc
- **License**: ISC
- **What we use**: C11 frontend that outputs QBE IL

### WLA-DX (Assembler/Linker)
- **Author**: Ville Helin
- **Repository**: https://github.com/vhelin/wla-dx
- **License**: GPL-2.0
- **What we use**: wla-65816 assembler, wla-spc700 for audio, wlalink linker
- **Note**: Used as a separate tool (not linked), compatible with MIT

## Audio

### SNESMOD
- **Author**: Mukunda Johnson
- **Repository**: https://github.com/mukunda-/snesmod
- **License**: MIT
- **What we use**: SPC700 audio driver (`sm_spc.asm`) and `smconv` tracker
  converter (Impulse Tracker `.it` → SPC700 soundbank)

## Vendored Dependencies

### LodePNG
- **Author**: Lode Vandevenne
- **License**: zlib
- **Version**: 20260119
- **Used in**: `tools/gfx4snes/`, `tools/img2snes/`
- **What it does**: PNG decoding/encoding


## Tools

### gfx4snes
- **Primary author**: Alekmaul (PVSnesLib)
- **License**: zlib
- **Additional contributors**:
  - **Neviksti** — pcx2snes conversion code (foundation of tile conversion)
  - **Artemio Urbina** — Palette rounding option
  - **Andrey Beletsky** — BMP BI_RLE8 compression support


## Game Assets

### Kenney "Pixel Shmup"
- **Author**: Kenney (https://kenney.nl/)
- **Pack**: https://kenney.nl/assets/pixel-shmup
- **License**: CC0 (Public Domain)
- **Used in**: `examples/games/shmup_1942/res/` —
  `tiles_packed.png`, `ships_packed.png`, and the `sprites.png` /
  `ground.png` split outputs derived from `tiles_packed.png` via
  `res/import.sh`.

## Documentation Sources
- **SNESdev Wiki** (https://snes.nesdev.org/) — CC-BY-SA
- **Fullsnes by Nocash** (https://problemkaputt.de/fullsnes.htm)
- **Super Famicom Development Wiki** (https://wiki.superfamicom.org/) — CC-BY-SA

## Special Thanks

- **RetroAntho** — For breathing new life into PVSnesLib when the project
  had gone quiet, and for welcoming new contributors with open arms. Without
  his energy, I probably never would have stopped and would have kept walking.

## Contributors

### OpenSNES
- **k0b3n4irb** — Fork maintainer, C11 compiler (QBE w65816 backend),
  library expansion, build system, testing infrastructure

## Code Attribution in Files

When code is directly derived from another project, the source file contains
a header comment:

```c
/*
 * Originally from: PVSnesLib (https://github.com/alekmaul/pvsneslib)
 * Author: Alekmaul
 * License: MIT
 * Modifications: [describe changes]
 */
```

## How to Attribute New Code

When adding code from external sources:

1. Add entry to this file
2. Include header comment in source file
3. Ensure license compatibility (MIT, BSD, ISC, zlib, Public Domain)
4. Document any modifications made

## License Compatibility

OpenSNES uses the MIT license. Compatible source licenses:
- MIT, BSD 2/3-clause, ISC, zlib (fully compatible)
- CC0 / Public Domain (compatible)
- CC-BY-SA (documentation only)

Note: WLA-DX is GPL-2.0 but used as a separate tool (not linked into any
binary), which is acceptable under GPL terms.

Incompatible (do not use without isolation):
- GPL (for library code — would require relicensing)
- Proprietary

## examples/graphics/effects/hdma_wave_table

- `res/water.bmp` — original work: procedurally generated water caustics
  (sum-of-sines field, 256-color indexed), created for this example.
  The HDMA technique it demonstrates is a C port of "SNES Wave HDMA Demo"
  by krom (Peter Lemon), github.com/PeterLemon/SNES — code technique
  credited, no krom assets used.

## examples/audio/pitch_mod

- `res/cello.brr` — the cello BRR sample shipped with krom (Peter
  Lemon)'s PitchMod demo (github.com/PeterLemon/SNES,
  `SPC700/PitchMod/BRR/`), used at his exact loop point (4167) and
  pitch. krom's repository carries no explicit license, and the
  recording's original source is unknown — revisit before any
  commercial redistribution. The companion 9-byte LFO square wave is
  transcribed in `player.spc700.asm` (technique, not an asset).

## examples/audio/soundboard

- `res/cello.brr` — same sample and caveat as
  `examples/audio/pitch_mod` (byte-identical copy).
- `res/ss.brr`, `res/pp.brr` — OpenSNES-original formant-synthesized
  phonemes (byte-identical copies from `examples/audio/speech_synth`,
  original work, MIT).
- `res/au.brr` — RMS-boosted regeneration of speech_synth's AU
  (original work, MIT; generator: `gen_au_boost.py` in the example).

## examples/audio/apu_switch

- `res/cello.brr` — same sample and caveat as
  `examples/audio/pitch_mod` above (byte-identical copy; the example
  hot-swaps between this repo's own drum and cello APU programs).
  krom's PlayTwoSong demo, whose *protocol* this example ports, ships
  songs built from samples ripped from commercial games — none of
  those assets are used here.
  `audio/play_noise` ships no assets at all (DSP noise generator only,
  technique credited to the same repository).
