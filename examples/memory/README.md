# Memory Examples

Learn SNES memory mapping features: HiROM mode for larger ROM bank windows, and
battery-backed SRAM for persistent save data.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [hirom_demo](hirom_demo/) | Intermediate | HiROM mode with 64KB banks (vs LoROM's 32KB) |
| [save_game](save_game/) | Intermediate | Battery-backed SRAM for saving and loading game state |

## Key Concepts

### LoROM vs HiROM

| Feature | LoROM | HiROM |
|---------|-------|-------|
| Bank size | 32KB ($8000-$FFFF) | 64KB ($0000-$FFFF) |
| Max ROM | 4MB | 4MB |
| Use case | Most games | Large contiguous data access |

Enable HiROM: set `USE_HIROM=1` in your Makefile.

### SRAM (Battery Backup)

| Feature | LoROM | HiROM |
|---------|-------|-------|
| Address | $70:0000-$7D:FFFF | $30:6000-$3F:7FFF |
| Common sizes | 2KB, 8KB, 32KB | 2KB, 8KB, 32KB |

Enable SRAM: set `USE_SRAM=1` in your Makefile. Write to the SRAM address range
to store data; it persists when the console is powered off (battery permitting).

---

Start with **hirom_demo** to understand memory mapping, then **save_game** for
persistent storage.
