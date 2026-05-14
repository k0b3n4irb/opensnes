# HDMA Detailed Notes

## Table Format: Repeat vs Direct Mode

### Repeat Mode (WORKS)
Each entry: `[count | 0x80] [data_lo] [data_hi]`
- Same data applied to `count` scanlines
- `0x81` = repeat 1 line = per-scanline control (3 bytes/line)
- `0xA0` = repeat 32 lines (0x80 | 0x20)
- 224 lines × 3 bytes + 1 end byte = 673 bytes for full-screen per-scanline

### Direct Mode (BROKEN on this toolchain)
Each entry: `[count] [data0_lo][data0_hi] [data1_lo][data1_hi] ...`
- Different data per scanline within the count
- Max count = 127 (7 bits)
- Tested with 112+112 split — produced no visible effect (straight lines)
- Root cause unknown — may be assembler/linker issue with table layout

## Working Example: Static Sine Wave (ROM)
```c
// Pre-computed 224-entry repeat-mode table
static const u8 hdma_sine_table[] = {
    0x81, lo0, hi0,  // line 0
    0x81, lo1, hi1,  // line 1
    ...
    0x81, lo223, hi223,  // line 223
    0x00  // end
};

// Direct register setup (bypass library)
*(vu8*)0x4360 = 0x02;    // DMAP6: mode 2 (write twice to same reg)
*(vu8*)0x4361 = 0x0D;    // BBAD6: BG1HOFS
*(vu8*)0x4362 = lo_byte;  // A1T6L: table addr low
*(vu8*)0x4363 = hi_byte;  // A1T6H: table addr high
*(vu8*)0x4364 = 0x00;     // A1B6: bank 0

// Enable/disable per frame
*(vu8*)0x420C = 0x40;  // enable channel 6
*(vu8*)0x420C = 0x00;  // disable all HDMA
```

## HDMA Enable/Disable
- Write `$420C` (HDMAEN) directly every frame — DON'T rely on library state tracking
- HDMA is re-initialized at V=0 each frame from channel registers
- Write during VBlank (after WaitForVBlank) takes effect next frame

## Bank Handling for Tables
- ROM tables (addr >= $8000): bank byte = $00 (LoROM bank 0)
- RAM tables (addr < $8000): bank byte = $7E for WRAM
- C code cannot write to bank $7E above $2000 — use WRAM port ($2180-$2183)

## Multiple Amplitude Tables (Working Pattern)
- 7 pre-computed ROM tables at different amplitudes (0,4,8,12,16,20,24 px)
- Use offset lookup table to avoid runtime multiplication
- Switch tables by updating A1T6L/A1T6H during VBlank
- State variables (amp_idx, button_held, wave_on) MUST be in WRAM ($0050+), not stack

### Static tables (no animation): 224 entries
- Each table: 673 bytes (224 entries × 3 + 1 end marker)
  ```c
  static const u16 amp_offsets[7] = { 0, 673, 1346, 2019, 2692, 3365, 4038 };
  ```

### Extended tables (with animation): 335 entries
- Each table: 1006 bytes (335 entries × 3 + 1 end marker)
- 335 = 224 visible + 111 wrapping (sine period - 1)
  ```c
  static const u16 amp_offsets[7] = { 0, 1006, 2012, 3018, 4024, 5030, 6036 };
  ```

## Animation by Pointer Offset
- Sine period = 112 scanlines. Phase increments by 1 each frame, wraps at 112
- HDMA pointer = `table_base + amp_offsets[idx] + phase * 3`
- Multiply by 3 via addition (`phase + phase + phase`) — avoids broken variable shifts
- Update A1T6L/A1T6H every frame when wave is active
- HDMA reads 224 entries from the offset position; V-blank naturally stops it
- No runtime trig — pure pointer arithmetic on pre-computed ROM data

## Required BG Setup for HDMA Wave Demo
- Must set $2107 (BG1SC) explicitly — consoleInit/setMode don't do it
- Must use proper 4bpp tile data for Mode 1 BG1
- Example: `*(vu8*)0x2107 = 0x04;` for tilemap at VRAM $0400
