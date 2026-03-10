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
