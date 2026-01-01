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

### 816-tcc (Initial Compiler)
- **Author**: Alekmaul, based on TinyCC by Fabrice Bellard
- **Repository**: https://github.com/alekmaul/tcc
- **License**: LGPL
- **What we use**: Initially used as compiler, to be replaced by QBE-based compiler
- **Status**: Temporary, will be replaced

## Future Components

### QBE Compiler Backend (Planned)
- **Author**: Quentin Carbonneaux
- **Repository**: https://c9x.me/compile/
- **License**: MIT
- **What we will use**: Compiler infrastructure for our 65816 backend
- **Status**: Planned - new 65816 backend to be developed

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
- CC0/Public Domain (compatible)
- LGPL (for tools/compiler, with care)
- CC-BY-SA (for documentation only)

Incompatible (do not use without isolation):
- GPL (viral, would require relicensing)
- Proprietary (obviously)
