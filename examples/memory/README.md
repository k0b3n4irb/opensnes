# Memory Examples

Learn SNES memory features: SRAM saves and HiROM mode.

## Examples

| Example | Complexity | Description |
|---------|------------|-------------|
| [2_save_game](2_save_game/) | 2 | Battery-backed SRAM for save games |
| [3_hirom_demo](3_hirom_demo/) | 3 | HiROM mode (64KB banks) |

## Key Concepts

### SRAM (Battery Backup)
- Location: $70:0000-$7D:FFFF (LoROM), $30:6000-$3F:7FFF (HiROM)
- Sizes: 2KB, 8KB, 32KB common
- Enable: Set USE_SRAM=1 in Makefile

### LoROM vs HiROM

| Feature | LoROM | HiROM |
|---------|-------|-------|
| Bank size | 32KB | 64KB |
| ROM start | $8000 | $0000 |
| Max size | 4MB | 4MB |
| Use case | Most games | Large data |

Enable HiROM: Set USE_HIROM=1 in Makefile
