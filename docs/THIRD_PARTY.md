# Third-Party Components

This document lists third-party tools and libraries included in OpenSNES, along with their original authors and licenses.

---

## gfx4snes

**Description**: SNES graphics converter for tiles, sprites, metasprites, and tilemaps.

**Original Author**: Alekmaul (https://github.com/alekmaul)

**Source**: [PVSnesLib](https://github.com/alekmaul/pvsneslib) - `tools/gfx4snes/`

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

**Notes**:
- Use the `-T -X -Y` flags for metasprite generation
- Critical bug fixes are in the develop branch (main branch has tile index issues)

---

## smconv

**Description**: SNES audio converter for SNESMOD tracker music format.

**Original Author**: Alekmaul (https://github.com/alekmaul)

**Source**: [PVSnesLib](https://github.com/alekmaul/pvsneslib) - `tools/smconv/`

**License**: MIT License (same as above)

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
