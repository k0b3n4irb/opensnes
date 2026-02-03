# Save Game Example

Battery-backed SRAM for persistent game saves.

## Learning Objectives

After this lesson, you will understand:
- SRAM structure and access
- Save validation with magic bytes
- Data integrity using checksums
- Detecting new vs existing saves

## Prerequisites

- Completed calculator example
- Understanding of memory layout

---

## What This Example Does

Demonstrates complete save system:
- Counter value that persists between power cycles
- Magic byte validation for save detection
- XOR checksum for data integrity
- Save/Load/Clear operations

```
+----------------------------------------+
|                                        |
|         SAVE GAME DEMO                 |
|                                        |
|         Counter: 00042                 |
|                                        |
|         [SAVED]                        |
|                                        |
|   Up/Down: Change value                |
|   A: Save    B: Load                   |
|   Select: Clear save                   |
|                                        |
+----------------------------------------+
```

**Controls:**
- D-Pad Up: Increment counter
- D-Pad Down: Decrement counter
- A button: Save to SRAM
- B button: Load from SRAM
- Select button: Clear save (reset to defaults)

---

## Code Type

**C with Direct SRAM Access**

| Component | Type |
|-----------|------|
| SRAM read/write | Library or direct access |
| Checksum | C implementation |
| Display | Direct VRAM writes |
| Input | Direct register access |

---

## SRAM Basics

### What is SRAM?

SRAM (Static RAM) is battery-backed memory:
- Retains data when power is off
- Typically 2KB-32KB
- Located at $70:0000-$7D:FFFF (LoROM)

### Enabling SRAM

In Makefile:
```makefile
USE_SRAM := 1
```

This sets the appropriate ROM header flags.

---

## Save Data Structure

### Magic Bytes

Use a signature to detect valid saves:

```c
#define SAVE_MAGIC "OSNS"

typedef struct {
    char magic[4];       /* "OSNS" */
    u16 counter;         /* Game data */
    u8 checksum;         /* Data integrity */
} SaveData;
```

### Validation

```c
u8 isSaveValid(void) {
    SaveData *save = (SaveData *)0x700000;

    /* Check magic bytes */
    if (save->magic[0] != 'O' ||
        save->magic[1] != 'S' ||
        save->magic[2] != 'N' ||
        save->magic[3] != 'S') {
        return 0;  /* No valid save */
    }

    /* Verify checksum */
    if (calculateChecksum(save) != save->checksum) {
        return 0;  /* Corrupted */
    }

    return 1;  /* Valid save */
}
```

---

## Checksum Calculation

### XOR Checksum

Simple but effective for basic validation:

```c
u8 calculateChecksum(SaveData *data) {
    u8 *bytes = (u8 *)data;
    u8 sum = 0;

    /* XOR all bytes except checksum itself */
    for (u8 i = 0; i < sizeof(SaveData) - 1; i++) {
        sum ^= bytes[i];
    }

    return sum;
}
```

---

## Save Operations

### Saving Data

```c
void saveGame(void) {
    SaveData *sram = (SaveData *)0x700000;

    /* Set magic bytes */
    sram->magic[0] = 'O';
    sram->magic[1] = 'S';
    sram->magic[2] = 'N';
    sram->magic[3] = 'S';

    /* Copy game data */
    sram->counter = game_counter;

    /* Calculate and store checksum */
    sram->checksum = calculateChecksum(sram);
}
```

### Loading Data

```c
void loadGame(void) {
    SaveData *sram = (SaveData *)0x700000;

    if (isSaveValid()) {
        game_counter = sram->counter;
        showMessage("LOADED");
    } else {
        showMessage("NO SAVE");
    }
}
```

### Clearing Save

```c
void clearSave(void) {
    SaveData *sram = (SaveData *)0x700000;

    /* Invalidate magic bytes */
    sram->magic[0] = 0;
    sram->magic[1] = 0;
    sram->magic[2] = 0;
    sram->magic[3] = 0;

    /* Reset to defaults */
    game_counter = 0;
    showMessage("CLEARED");
}
```

---

## Build and Run

```bash
cd examples/basics/2_save_game
make clean && make

# Run in emulator (enable SRAM saving)
/path/to/Mesen save_game.sfc
```

Note: Configure your emulator to save SRAM to disk.

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Save system implementation |
| `data.asm` | Font and display data |
| `Makefile` | Build config with USE_SRAM=1 |

---

## Exercises

### Exercise 1: Multiple Save Slots

Implement 3 save slots:
```c
#define SAVE_SLOT_SIZE 64
#define SRAM_BASE 0x700000

SaveData *getSlot(u8 slot) {
    return (SaveData *)(SRAM_BASE + slot * SAVE_SLOT_SIZE);
}
```

### Exercise 2: Last Modified

Track save timestamp (using frame counter as pseudo-time):
```c
typedef struct {
    char magic[4];
    u32 play_time;   /* Frames played */
    u16 counter;
    u8 checksum;
} SaveData;
```

### Exercise 3: Backup Slot

Maintain a backup copy:
```c
void saveGame(void) {
    /* Copy current save to backup slot first */
    memcpy(&sram[1], &sram[0], sizeof(SaveData));
    /* Then save new data to main slot */
    ...
}
```

### Exercise 4: CRC Checksum

Implement more robust CRC16:
```c
u16 crc16(u8 *data, u16 len) {
    u16 crc = 0xFFFF;
    /* CRC calculation... */
    return crc;
}
```

---

## Technical Notes

### SRAM Banks

In LoROM:
```
$70:0000-$70:7FFF = SRAM (if 32KB)
$70:0000-$70:1FFF = SRAM (if 8KB)
```

### Battery Life

Real cartridge batteries last 15-25 years. Emulators save SRAM to disk files.

### Corruption Prevention

- Always validate before loading
- Use checksums for integrity
- Consider backup slots
- Don't trust magic bytes alone

### ROM Header

The ROM header at $FFD8 indicates SRAM size:
```
$FFD8 bit 0-2: SRAM size
  0 = No SRAM
  1 = 2KB
  3 = 8KB
  5 = 32KB
```

---

## What's Next?

**Collision:** [Collision Demo](../3_collision_demo/) - Hit detection

**Movement:** [Smooth Movement](../4_smooth_movement/) - Fixed-point math

---

## License

Code: MIT
