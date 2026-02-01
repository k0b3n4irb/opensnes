# HDMA Gradient Example

A per-scanline brightness gradient using HDMA and color math.

## Learning Objectives

After this lesson, you will understand:
- What HDMA (Horizontal DMA) is and how it works
- HDMA table format and structure
- Color math fixed color registers
- How to combine HDMA with color math for visual effects
- The difference between 16-bit C pointers and 24-bit ROM addresses

## Prerequisites

- Understanding of DMA basics
- Completed background examples

---

## What This Example Does

Displays a background image with a vertical brightness gradient applied in real-time:
- Top of screen is dark
- Bottom of screen is bright
- Gradient transitions smoothly across 16 intensity levels

```
+----------------------------------------+
|  DARK                                  |
|                                        |
|  MEDIUM                                |
|                                        |
|  BRIGHT                                |
+----------------------------------------+
```

The effect is achieved by:
1. Enabling color math in "add fixed color" mode
2. Using HDMA to change the fixed color intensity each scanline group

---

## Code Type

**C + Assembly**

| Component | Type |
|-----------|------|
| Main setup | C (`main.c`) |
| HDMA table | Assembly (`data.asm`) |
| HDMA functions | Assembly (`hdma.asm` in lib) |
| Color math init | C (direct register writes) |
| Graphics loading | Library (`bgInitTileSet`, `dmaCopyVram`) |

---

## HDMA Fundamentals

### What is HDMA?

HDMA (Horizontal-blanking DMA) is a special DMA mode that:
- Transfers a small amount of data once per scanline
- Happens during the brief horizontal blanking period
- Can write to any PPU register
- Allows per-scanline visual effects

### HDMA Table Format

HDMA uses a table in ROM that specifies what data to write:

```
Byte 0: Line count (1-127 for direct mode)
Byte 1+: Data to write (1-4 bytes depending on mode)
... repeat ...
Byte N: 0 (end of table marker)
```

Example from this demo:
```asm
hdma_gradient_table:
    .db 14, $E0 | 0       ; 14 lines at intensity 0
    .db 14, $E0 | 1       ; 14 lines at intensity 1
    .db 14, $E0 | 2       ; etc.
    ; ...
    .db 0                 ; End marker
```

### HDMA Transfer Modes

| Mode | Bytes | Use Case |
|------|-------|----------|
| `HDMA_MODE_1REG` | 1 | Single register (like COLDATA) |
| `HDMA_MODE_1REG_2X` | 2 | Same register twice (scroll registers) |
| `HDMA_MODE_2REG` | 2 | Two consecutive registers |

---

## Color Math

### COLDATA Register ($2132)

The COLDATA register sets the "fixed color" used in color math:

```
Bit 7: Blue channel enable
Bit 6: Green channel enable
Bit 5: Red channel enable
Bits 4-0: Intensity (0-31)
```

To set all channels at once, use `$E0 | intensity`:
- `$E0` = bits 7,6,5 all set (write to R, G, B simultaneously)
- This creates grayscale values (R=G=B)

### Color Math Registers

```c
REG_CGWSEL = 0x02;      /* Use fixed color as source */
REG_CGADSUB = 0x01;     /* Add mode, enable for BG1 */
```

---

## HDMA Setup

### Setting Up an HDMA Channel

```c
/* Channel 7, 1-byte transfer, target COLDATA register */
hdmaSetup(HDMA_CHANNEL_7, HDMA_MODE_1REG, HDMA_DEST_COLDATA, hdma_gradient_table);

/* Enable channel 7 */
hdmaEnable(1 << HDMA_CHANNEL_7);
```

### Why Assembly is Required

The HDMA table address must include a 24-bit bank byte. C code on the 65816 only handles 16-bit pointers, so the assembly implementation extracts the correct bank:

```asm
; If address >= $8000, it's ROM in bank 0 (LoROM)
lda 6,s                 ; High byte of table address
cmp #$80
bcc @use_ram_bank       ; < $8000 means RAM
lda #$00                ; ROM: use bank 0
bra @set_bank
@use_ram_bank:
lda #$7E                ; RAM: use bank $7E
@set_bank:
sta.l $0004,x           ; Write to A1B (bank byte)
```

---

## The Gradient Table

```asm
hdma_gradient_table:
    ; Top - dark (low intensity on all channels)
    .db 14, $E0 | 0       ; All channels = 0
    .db 14, $E0 | 1       ; All channels = 1
    .db 14, $E0 | 2       ; All channels = 2
    .db 14, $E0 | 3       ; All channels = 3

    ; Upper middle - getting brighter
    .db 14, $E0 | 4
    .db 14, $E0 | 5
    .db 14, $E0 | 6
    .db 14, $E0 | 7

    ; Middle
    .db 14, $E0 | 8
    .db 14, $E0 | 9
    .db 14, $E0 | 10
    .db 14, $E0 | 11

    ; Lower - bright
    .db 14, $E0 | 12
    .db 14, $E0 | 13
    .db 14, $E0 | 14
    .db 28, $E0 | 15      ; Last section (more lines)

    .db 0                 ; End of table
```

Each entry: `14 lines` at `intensity N`, covering 224 scanlines total (14 x 16 = 224).

---

## Build and Run

```bash
cd examples/graphics/16_hdma_gradient
make clean && make

# Run in emulator
/path/to/Mesen hdma_gradient.sfc
```

---

## Files

| File | Purpose |
|------|---------|
| `main.c` | Setup background, color math, and HDMA |
| `data.asm` | Graphics data and HDMA gradient table |
| `Makefile` | Build configuration |
| `res/pvsneslib.bmp` | Source background image |

---

## Common HDMA Pitfalls

### 1. Black Screen

If you get a black screen with HDMA:
- Check that the table has a proper `0` terminator
- Verify the bank byte is correct (assembly handles this)
- Ensure color math is configured correctly

### 2. Wrong Colors

If colors are wrong (yellow, pink instead of gray gradient):
- Make sure you're setting ALL color channels with `$E0`
- Initialize all channels to 0 before starting

### 3. Garbage on Screen

If you see random patterns:
- Check HDMA table format (line count + data bytes)
- Verify transfer mode matches data size

---

## Exercises

### Exercise 1: Color Gradient

Change the gradient from grayscale to a color fade:
```asm
; Red-only gradient
.db 14, $20 | 0       ; Red channel only ($20 instead of $E0)
.db 14, $20 | 4
; ...
```

### Exercise 2: Finer Gradient

Create a smoother gradient with more steps:
```asm
; 7 lines per step instead of 14
.db 7, $E0 | 0
.db 7, $E0 | 1
.db 7, $E0 | 2
; ... (32 steps for full range)
```

### Exercise 3: Animated Gradient

Use two HDMA tables and swap between them each frame:
```c
static u8 table_index = 0;
table_index = !table_index;
hdmaSetTable(HDMA_CHANNEL_7,
    table_index ? hdma_table_a : hdma_table_b);
```

### Exercise 4: Sky Effect

Create a blue sky gradient:
```asm
; Blue channel only, increasing intensity
.db 14, $80 | 4       ; Dark blue at top
.db 14, $80 | 8
.db 14, $80 | 12
.db 14, $80 | 16      ; Bright blue at bottom
```

---

## Technical Notes

### HDMA Channels

- Use channels 6-7 for HDMA (0-5 typically used for regular DMA)
- Multiple HDMA channels can run simultaneously
- Each channel can target different registers

### Timing

- HDMA transfers happen during horizontal blanking
- The table pointer is read at the start of each frame
- Changing the table mid-frame requires double buffering

### DMA Channel Conflict

Regular DMA and HDMA can use different channels simultaneously, but:
- Don't use the same channel for both
- HDMA channels stay active across frames
- Disable HDMA before using its channel for regular DMA

---

## What's Next?

**Color Math:** [Transparency Example](../17_transparency/) - Shadow and tint effects

**Window Masking:** [Window Example](../18_window/) - Spotlight effect

---

## License

Code: MIT
