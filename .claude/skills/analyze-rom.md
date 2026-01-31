# /analyze-rom - Analyze SNES ROM

Analyze a SNES ROM file for debugging or reverse engineering.

## Usage
```
/analyze-rom <path>              # Analyze ROM file
/analyze-rom tone.sfc            # Example
/analyze-rom --header tone.sfc   # Show header info only
```

## Implementation

### ROM Header Analysis
Read bytes at $00FFC0-$00FFFF (LoROM) or $40FFC0-$40FFFF (HiROM):

```
Offset  Size  Description
------  ----  -----------
$FFC0   21    Game title (ASCII, space-padded)
$FFD5   1     ROM makeup (LoROM/HiROM/FastROM)
$FFD6   1     ROM type (ROM, ROM+RAM, ROM+RAM+SRAM)
$FFD7   1     ROM size (2^N KB)
$FFD8   1     SRAM size (2^N KB, 0=none)
$FFD9   1     Country code
$FFDA   1     Developer ID
$FFDB   1     Version
$FFDC   2     Checksum complement
$FFDE   2     Checksum
```

### Symbol File Analysis
If `.sym` file exists alongside ROM:
```bash
# Show symbol table
cat <rom>.sym | head -50
```

### Size Analysis
```bash
# ROM size
ls -la <rom>.sfc

# Code vs data breakdown (estimate from sections)
grep -E "^[0-9A-F]+:" <rom>.sym | wc -l
```

### Mesen2 Debug Launch
```bash
# Launch in debugger mode (Mesen must be in PATH)
Mesen --debugger <rom>.sfc &
```

## Output
Report:
1. ROM header information
2. File size and checksum
3. Symbol count (if .sym exists)
4. Any anomalies detected
