# HiROM Demo

Demonstrates HiROM memory mapping mode on the SNES.

## What is HiROM?

The SNES supports two primary ROM mapping modes:

| Aspect | LoROM (default) | HiROM |
|--------|-----------------|-------|
| Bank Size | 32KB ($8000) | 64KB ($10000) |
| ROM Address | $8000-$FFFF | $0000-$FFFF |
| Banks for 256KB | 8 | 4 |
| Cartridge Type | $00 (ROM) | $21 (ROM) |

## When to Use HiROM

HiROM is useful when:
- You need large contiguous data access (64KB without bank switching)
- Your game has large assets like uncompressed graphics or audio
- Simpler address math is preferred over optimal memory utilization

LoROM is better when:
- You want maximum compatibility
- Your game fits in smaller ROM sizes
- You need SRAM (HiROM SRAM support is limited in this SDK version)

## Usage

Add to your Makefile:

```makefile
USE_HIROM := 1
```

That's it! The build system handles the rest.

## Verification

After building, you can verify HiROM mode:

```bash
# Check ROM header byte at offset $FFD5
xxd -s 0xFFD5 -l 1 hirom_demo.sfc
# Should show: 21 (HiROM)

# In Mesen2, the ROM info should show "HiROM"
```

## Limitations

- `USE_SRAM + USE_HIROM` is not fully supported yet (SRAM is at different address in HiROM)
- FastROM is not yet implemented

## Building

```bash
make
```

## Files

- `main.c` - Simple demo that displays "HIROM MODE"
- `Makefile` - Build configuration with `USE_HIROM := 1`
