# Third-Party Components {#third_party}

This page describes the main third-party components OpenSNES builds on, with their authors and licences. `ATTRIBUTION.md` is the complete list (the image and map libraries of the tools — LodePNG, stb_image, cute_tiled — and modlib among them).

---

## gfx4snes

**Description**: SNES graphics converter for tiles, sprites, metasprites, and tilemaps.

**Original Author**: Alekmaul (https://github.com/alekmaul)

**Source**: [PVSnesLib](https://github.com/alekmaul/pvsneslib) - `tools/lib/gfx4snes/`

**Version**: 2.0.0 (develop branch, 2025-01-25)

**License**: MIT License

```
MIT License

Copyright (c) 2012-2023 alekmaul

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

**Embedded in**: `opensnes-tileset` and `opensnes-sprite`, which link its
converter sources (`tools/lib/gfx4snes/src/`: images, palettes, tiles, maps,
metasprites, lz77, loadbmp) as a library; their `.pic`, `.map`, `.pal` and
`_meta.inc` are byte for byte gfx4snes's. The standalone `gfx4snes` is
legacy (@ref tools_legacy).

**Notes**:
- Critical bug fixes are in the develop branch (main branch has tile index issues)

---

## SNESMOD driver

**Description**: the SPC700 driver that plays tracker music and sound
effects (`USE_SNESMOD`), uploaded by `snesmodInit()`.

**Original Author**: Mukunda Johnson (https://github.com/mukunda-/snesmod),
with bugfixes by KungFuFurby

**Source**: `lib/source/sm_spc.spc700.asm`, built by `wla-spc700` at every
library build: the driver's source in the WLA-DX syntax
[PVSnesLib](https://github.com/alekmaul/pvsneslib) converted it to
(`pvsneslib/snesmod/sm_spc_wla.asm`), with one OpenSNES fix (the voices'
key-off in `ResetSound`).

**License**: MIT License (the notice is in the source file)

---

## smconv

**Description**: SNES audio converter for SNESMOD tracker music format.

**Original Author**: Mukunda Johnson (SNESMOD), adapted by Alekmaul for
PVSnesLib (https://github.com/alekmaul)

**Source**: [PVSnesLib](https://github.com/alekmaul/pvsneslib) - `tools/lib/smconv/`

**License**: MIT License (same as above)

**Embedded in**: `opensnes-music`, which links the converter sources of
`tools/lib/smconv/src/` (IT loader, IT-to-SPC conversion, BRR encoder, report),
and `opensnes-sample`, which links its BRR encoder (`brr.c`). The build
converts `SOUNDBANK_SRC` with `opensnes-music`.

---

## WLA-DX

**Description**: Assembler and linker for 65816 and other processors.

**Original Author**: Ville Helin

**Source**: https://github.com/vhelin/wla-dx

**License**: GPL v2

---

## cproc

**Description**: C11 compiler using QBE as backend.

**Original Author**: Michael Forney

**Source**: https://git.sr.ht/~mcf/cproc

**License**: ISC License

---

## QBE (w65816 backend)

**Description**: Compiler backend with 65816 target support.

**Original Author**: Quentin Carbonneaux (QBE), 65816 backend modifications for SNES

**Source**: https://c9x.me/compile/

**License**: MIT License
