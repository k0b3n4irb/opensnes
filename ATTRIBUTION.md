# Attribution

OpenSNES is a spiritual successor to PVSnesLib, building upon the work of many contributors in the SNES homebrew community. We are grateful for their pioneering efforts.

## Primary Lineage

### PVSnesLib
- **Author**: Alekmaul and contributors
- **Repository**: https://github.com/alekmaul/pvsneslib
- **License**: MIT
- **What we use**:
  - Core assembly library concepts (sprites, backgrounds, DMA)
  - Tool architecture inspiration (graphics converters, sound tools)
  - Build system patterns
- **Status**: OpenSNES is inspired by PVSnesLib but is a separate project with different goals

## Compiler Stack

### QBE (Query Compiler Backend)
- **Author**: Quentin Carbonneaux
- **Repository**: https://c9x.me/compile/
- **License**: MIT
- **What we use**: Compiler infrastructure with our custom w65816 backend
- **Status**: Active - our 65816 backend generates working SNES code

### cproc (C Compiler Frontend)
- **Author**: Michael Forney
- **Repository**: https://git.sr.ht/~mcf/cproc
- **License**: ISC
- **What we use**: C11 frontend that outputs QBE IL
- **Status**: Active - integrated with our QBE backend

### WLA-DX (Assembler/Linker)
- **Author**: Ville Helin
- **Repository**: https://github.com/vhelin/wla-dx
- **License**: GPL-2.0
- **What we use**: wla-65816 assembler, wla-spc700 for audio, wlalink
- **Status**: Active - used for all assembly and linking

## Historical

### 816-tcc (No Longer Used)
- **Author**: Alekmaul, based on TinyCC by Fabrice Bellard
- **Repository**: https://github.com/alekmaul/tcc
- **License**: LGPL
- **What we used**: Initially considered as compiler
- **Status**: Removed - replaced by QBE-based cc65816

## Documentation Sources

### SNESdev Wiki
- **URL**: https://snes.nesdev.org/
- **License**: CC-BY-SA
- **What we reference**: Hardware documentation, register descriptions

### Fullsnes by Nocash
- **URL**: https://problemkaputt.de/fullsnes.htm
- **What we reference**: Comprehensive SNES hardware documentation

### Super Famicom Development Wiki
- **URL**: https://wiki.superfamicom.org/
- **License**: CC-BY-SA
- **What we reference**: Programming tutorials, hardware info

## Code Attribution in Files

When code is directly derived from another project, the source file contains a header comment:

```c
/*
 * Originally from: PVSnesLib (https://github.com/alekmaul/pvsneslib)
 * Author: Alekmaul
 * License: MIT
 * Modifications: [describe changes]
 */
```

## Contributors

### OpenSNES Team
- [Your name/handle here]

### Original PVSnesLib Contributors
See: https://github.com/alekmaul/pvsneslib/graphs/contributors

## How to Attribute New Code

When adding code from external sources:

1. Add entry to this file
2. Include header comment in source file
3. Ensure license compatibility (MIT, BSD, LGPL, CC-BY-SA)
4. Document any modifications made

## License Compatibility

OpenSNES uses MIT license. Compatible source licenses:
- MIT (fully compatible)
- BSD 2/3-clause (compatible)
- ISC (compatible)
- CC0/Public Domain (compatible)
- CC-BY-SA (for documentation only)

Note: WLA-DX is GPL-2.0 but used as a separate tool (not linked), which is acceptable.

Incompatible (do not use without isolation):
- GPL (for library code - would require relicensing)
- Proprietary (obviously)
