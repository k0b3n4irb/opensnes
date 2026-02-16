# Save Game — SRAM Persistence

## What This Example Shows

How to save and load game data using the SNES cartridge SRAM (Static RAM). The SRAM
is battery-backed — data persists even when the console is powered off. This is how
classic SNES games like Zelda and Final Fantasy save progress.

## Prerequisites

Read `text/hello_world` first (console setup, text display).

## Controls

| Button | Action |
|--------|--------|
| A | Write test data to Slot 1 |
| B | Read and display Slot 1 |
| X | Write test data to Slot 2 |
| Y | Read and display Slot 2 |

## How It Works

### 1. Define a save structure

```c
typedef struct {
    s16 posX, posY;
    u16 camX, camY;
} SaveState;
```

The structure is 8 bytes. In a real game, this would hold player position, inventory,
level progress, etc.

### 2. Save to SRAM

```c
sramSaveOffset((u8 *)&vts, SAVE_SIZE, SAVE_SIZE * SLOT1);
```

`sramSaveOffset()` copies `SAVE_SIZE` bytes from RAM to SRAM at the given offset.
Multiple save slots use different offsets (Slot 0 at offset 0, Slot 1 at offset 8, etc.).

### 3. Load from SRAM

```c
sramLoadOffset((u8 *)&vtl, SAVE_SIZE, SAVE_SIZE * SLOT1);
```

`sramLoadOffset()` copies bytes back from SRAM into a RAM structure. The loaded
values are then displayed as hex on screen.

### 4. Enable SRAM in the ROM header

In the Makefile:
```makefile
USE_SRAM = 1
```

This sets the ROM header flag telling the emulator (or real hardware) that the
cartridge has battery-backed SRAM.

## SNES Concepts

- **SRAM**: 8 KB of battery-backed RAM at `$70:0000-$71:FFFF` (LoROM) or
  `$20:6000-$3F:7FFF` (HiROM). Data survives power cycles.
- **Save slots**: Just offsets into SRAM. With 8 KB and 8-byte saves, you could
  have over 1000 slots — but real games typically use 3-4 with larger structures.
- **Checksum**: This example doesn't validate saves. A real game should store a
  checksum to detect corrupted SRAM (weak battery, first boot, etc.).

## Next Steps

- `memory/hirom_demo` — Understand LoROM vs HiROM memory mapping
- `games/breakout` — See save/load in a game context (high scores)
