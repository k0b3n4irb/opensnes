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
- **What we use**: the SPC700 audio driver, built from its source
  `lib/source/sm_spc.spc700.asm` — Mukunda Johnson's `sm_spc.as7`, with
  KungFuFurby's bugfixes, in the WLA-DX syntax PVSnesLib converted it to
  (`pvsneslib/snesmod/sm_spc_wla.asm`, MIT) — and the converter of
  `smconv` (Impulse Tracker `.it` → SPC700 soundbank), built into
  `opensnes-music`

### modlib
- **Author**: Mukunda Johnson
- **Repository**: https://github.com/mukunda-/modlib (commit `2dc5fcc`)
- **License**: MIT
- **What we use**: the IT 2.14 / 2.15 sample decompressor of
  `itmod/itsamplecodec.go` and `itmod/bitstream.go`, ported to C in
  `tools/lib/smconv/src/itloader.c` (decoder only), and the test module
  `itmod/test/reflection.it`, copied as
  `tools/lib/smconv/tests/fixtures/reflection.it`
- **Licence text**:

  > Copyright 2025 Mukunda Johnson (mukunda.com)
  >
  > Permission is hereby granted, free of charge, to any person obtaining a
  > copy of this software and associated documentation files (the
  > "Software"), to deal in the Software without restriction, including
  > without limitation the rights to use, copy, modify, merge, publish,
  > distribute, sublicense, and/or sell copies of the Software, and to
  > permit persons to whom the Software is furnished to do so, subject to
  > the following conditions:
  >
  > The above copyright notice and this permission notice shall be included
  > in all copies or substantial portions of the Software.
  >
  > THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  > OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  > MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  > IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  > CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  > TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  > SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Vendored Dependencies

### LodePNG
- **Author**: Lode Vandevenne
- **License**: zlib
- **Version**: 20260119
- **Location**: `tools/third_party/lodepng.{c,h}`
- **Used in**: `opensnes-tileset`, `opensnes-sprite`, `opensnes-palette`,
  `opensnes-image` and `opensnes-text` (the PNG-reading tools)
- **What it does**: PNG decoding/encoding

### stb_image
- **Author**: Sean Barrett (nothings.org)
- **License**: public domain (Unlicense) or MIT, at the user's choice
- **Version**: 2.30
- **Location**: `tools/third_party/stb_image.h`
- **What it does**: image loading; no shipped tool links it today (the
  font converter that did is retired), its fuzz harness still runs

### cute_tiled
- **Author**: Randy Gaul; embeds a component by Mattias Gustavsson
- **License**: zlib or public domain (Unlicense) for cute_tiled; MIT or
  public domain (Unlicense) for the embedded component — both blocks at the
  end of the file
- **Version**: 1.06
- **Location**: `tools/third_party/cute_tiled.h`
- **What it does**: Tiled `.tmx` / `.tmj` map parsing for `opensnes-level`


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
- **Used in**: `examples/demos/shmup_1942/res/` —
  `tiles_packed.png`, `ships_packed.png`, and the `sprites.png` /
  `ground.png` split outputs derived from `tiles_packed.png` via
  `res/import.sh`.

### Kenney "Digital Audio" and "Interface Sounds"
- **Author**: Kenney (https://kenney.nl/)
- **Packs**: https://kenney.nl/assets/digital-audio,
  https://kenney.nl/assets/interface-sounds
- **License**: CC0 (Public Domain); each pack's `License.txt` is kept in
  `devtools/audio_assets/kenney/` beside the OGG files the examples use
- **Used in**: the effects of `examples/audio/snesmod_music`
  (`sfx/neo_lumen_sfx.it`: `phaseJump1`, `powerUp1`, `laser1`,
  `confirmation_001`) and the jump of `examples/games/likemario`
  (`res/jump_sfx.it`: `phaseJump1`), cut and converted by
  `devtools/audio_assets/build.py`.

### Music and instruments composed for OpenSNES
- **License**: CC0 (original work)
- **What**: every tracker module of the examples — `snesmod_music`'s
  "Neo-Lumen Nocturne", `blocks`' Korobeiniki (the 19th-century Russian
  song, public domain, arranged here), likemario's "Neo-Lumen Rooftops",
  the large-bank fixture's long nocturne — and their instruments, and the
  cello of `apu_switch` and `soundboard`, all synthesized by
  `devtools/audio_assets/` (`make audio-assets` writes them again).

### Néo-Lumen art generated for OpenSNES
- **License**: CC0 (Public Domain)
- **What**: the pictures, sprites and tiles the examples are being redrawn
  with, AI-generated with PixelLab (https://www.pixellab.ai/) for OpenSNES,
  some edited by hand. The originals, the tool and the seed of each, the
  edits and the examples that use them are listed in
  `assets/neo-lumen/README.md`.

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

## examples/hdma/hdma_wave

- `res/water.bmp` — original work: procedurally generated water caustics
  (sum-of-sines field, 256-color indexed), created for this example.
  The HDMA technique it demonstrates is a C port of "SNES Wave HDMA Demo"
  by krom (Peter Lemon), github.com/PeterLemon/SNES — code technique
  credited, no krom assets used. `res/wavetable.bin` is krom's HDMA
  table, extracted verbatim from the demo (data of the technique,
  credited).

## examples/audio/soundboard

- `res/cello.brr` — the same synthesized cello as
  `examples/audio/apu_switch` (byte-identical copy, original work).
- `res/ss.brr`, `res/pp.brr` — OpenSNES-original formant-synthesized
  phonemes (byte-identical copies from `examples/audio/speech_synth`,
  original work, MIT).
- `res/au.brr` — RMS-boosted regeneration of speech_synth's AU
  (original work, MIT; generator: `gen_au_boost.py` in the example).

## starter

- `res/player.png` — OpenSNES-original sprite art (a 32×32 indexed character),
  drawn programmatically for the starter template (original work, MIT). Meant to
  be replaced with your own.

## examples/audio/sfx_from_wav

- `res/blip.wav`, `res/coin.wav` — OpenSNES-original synthesized sound
  effects (a swept square-wave blip and a two-note pickup), generated
  deterministically for this example (original work, MIT). The `.brr`
  files are not committed — the build converts each `.wav` with wav2brr.

## examples/audio/apu_switch

- `res/cello.brr` — synthesized for OpenSNES by
  `devtools/audio_assets/build.py` (original work, CC0). It replaced the
  cello sample of krom (Peter Lemon)'s PitchMod demo, whose repository
  carries no explicit licence. The companion 9-byte LFO square wave of
  krom's demo is transcribed in `cello.spc700.asm` (technique, not an
  asset).
- The drum program ships no assets at all (the DSP noise generator
  only; PlayNoise technique credited to the same repository).
- krom's PlayTwoSong demo, whose *protocol* this example ports, ships
  songs built from samples ripped from commercial games — none of
  those assets are used here.
