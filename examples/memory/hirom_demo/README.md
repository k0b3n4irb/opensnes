# HiROM Demo — Understanding SNES Memory Mapping

## What This Example Shows

The difference between **LoROM** and **HiROM** memory mapping on the SNES, and how
to build a HiROM ROM with OpenSNES. The program displays "HIROM MODE" and changes
the background color when A is pressed — proving the ROM, library, and input all
work correctly in HiROM mode.

## Prerequisites

Read `text/hello_world` first (basic PPU setup). Understanding of hexadecimal
addressing is helpful but not required.

## Controls

| Button | Action |
|--------|--------|
| A (hold) | Changes background to light blue |

## How It Works

### 1. Enable HiROM in the Makefile

```makefile
USE_HIROM = 1
```

This tells the build system to:
- Use `hdr_hirom.asm` instead of `hdr.asm` for the ROM header
- Set the mapping mode byte to HiROM
- Adjust the linker configuration for 64 KB banks

### 2. The code is identical to LoROM

The C code doesn't need to change between LoROM and HiROM — the compiler and
linker handle the address mapping transparently. `consoleInit()`, `dmaCopyVram()`,
`padHeld()` — everything works the same way.

### 3. Embedded font (no external assets)

This demo uses a built-in 2bpp font defined as a `const` array in C:

```c
static const u8 font_tiles[] = { ... };
```

The `const` qualifier places the data in ROM (not RAM), so it works regardless
of memory mapping mode. The tiles are loaded to VRAM via `dmaCopyVram()`.

## SNES Concepts

### LoROM vs HiROM

| | LoROM | HiROM |
|------|-------|-------|
| Bank size | 32 KB ($8000-$FFFF) | 64 KB ($0000-$FFFF) |
| ROM range | Banks $00-$7D | Banks $C0-$FF |
| Max size | ~4 MB | ~4 MB |
| WRAM access | Banks $00-$3F, $80-$BF | Banks $00-$3F |

- **LoROM**: Each bank only exposes the upper 32 KB of ROM. The lower half
  ($0000-$7FFF) maps to hardware registers, WRAM, etc. Most SNES games use LoROM.
- **HiROM**: Each bank exposes the full 64 KB of ROM. This makes large data tables
  simpler to address (no bank-boundary splits). Used by games with large assets
  (Chrono Trigger, Star Ocean).

### When to use HiROM

- Large contiguous data (> 32 KB tilesets, music, maps)
- Simpler address math for streaming systems
- If you don't need it, stick with LoROM — it's the default and better tested

## Next Steps

- `memory/save_game` — SRAM persistence (works in both LoROM and HiROM)
- `games/likemario` — A larger project that benefits from understanding memory layout
