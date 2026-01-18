# Lesson 2: Custom Font

Display text using a complete embedded font with letters, numbers, and punctuation.

## Learning Objectives

After this lesson, you will understand:
- Building a complete font tileset (A-Z, 0-9, punctuation)
- Pre-encoding messages as tile indices
- Creating reusable text rendering functions
- The `font2snes` tool for asset conversion (alternative approach)

## Prerequisites

- Completed [1. Hello World](../1_hello_world/)
- Understanding of 2bpp tile format

---

## What This Example Does

Displays multiple text elements on screen:
- A title bar: `=== CUSTOM FONT ===`
- "HELLO WORLD!"
- The complete alphabet: A-Z
- Numbers: 0-9
- "OPENSNES" logo text

```
+----------------------------------------+
|                                        |
|      === CUSTOM FONT ===               |
|                                        |
|         HELLO WORLD!                   |
|                                        |
|          ABCDEFGHIJ                    |
|          KLMNOPQRST                    |
|            UVWXYZ                      |
|                                        |
|          0123456789                    |
|                                        |
|           OPENSNES                     |
+----------------------------------------+
```

---

## Code Type

**Mixed C + Direct Register Access**

| Component | Type |
|-----------|------|
| Hardware init | Library (`consoleInit()`) |
| Video mode | Library (`setMode()`) |
| Screen enable | Library (`setScreenOn()`) |
| VBlank sync | Library (`WaitForVBlank()`) |
| Font tiles | C const array (42 tiles, 672 bytes) |
| Text rendering | Helper function (`print_msg()`) |
| VRAM writes | Direct register access |

---

## The Font Tileset

The font contains 42 tiles covering essential characters:

| Index | Character |
|-------|-----------|
| 0 | Space |
| 1-26 | A-Z |
| 27-36 | 0-9 |
| 37 | ! |
| 38 | . |
| 39 | : |
| 40 | - |
| 41 | = |

Each tile is 16 bytes (8x8 pixels, 2bpp format):

```c
static const u8 font_tiles[] = {
    /* 0: Space */
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00,
    /* 1: A */
    0x18,0x00, 0x3C,0x00, 0x66,0x00, 0x7E,0x00,
    0x66,0x00, 0x66,0x00, 0x66,0x00, 0x00,0x00,
    /* 2: B */
    0x7C,0x00, 0x66,0x00, 0x7C,0x00, 0x66,0x00,
    0x66,0x00, 0x66,0x00, 0x7C,0x00, 0x00,0x00,
    /* ... more tiles ... */
};
```

---

## Pre-Encoded Messages

Instead of runtime character conversion (which causes compiler issues), messages are stored as arrays of tile indices:

```c
/* "=== CUSTOM FONT ===" */
static const u8 msg_title[] = {
    41,41,41, 0, 3,21,19,20,15,13, 0, 6,15,14,20, 0, 41,41,41, 0xFF
};
/*  =  =  =  SP C  U  S  T  O  M  SP F  O  N  T  SP =  =  =  END */

/* "HELLO WORLD!" */
static const u8 msg_hello[] = {
    8,5,12,12,15, 0, 23,15,18,12,4, 37, 0xFF
};
/* H E  L  L  O  SP W  O  R  L  D  !  END */
```

The `0xFF` byte marks the end of each message.

---

## The Text Rendering Function

A simple helper function writes messages to the tilemap:

```c
static void print_msg(u16 addr, const u8 *msg) {
    u16 i;

    REG_VMADDL = addr & 0xFF;
    REG_VMADDH = addr >> 8;

    i = 0;
    while (msg[i] != 0xFF) {
        REG_VMDATAL = msg[i];  /* Tile index */
        REG_VMDATAH = 0;       /* Attributes (palette 0) */
        i++;
    }
}
```

### Calculating Tilemap Addresses

The tilemap is at VRAM $0400. Each row has 32 tiles.

```c
/* Address = base + (row * 32) + column */
print_msg(0x0400 + 3*32 + 6, msg_title);    /* Row 3, column 6 */
print_msg(0x0400 + 7*32 + 10, msg_hello);   /* Row 7, column 10 */
```

---

## Alternative: Using font2snes

For larger projects, you can use the `font2snes` tool to convert a PNG font image:

### Create a Font Image

Requirements:
- Dimensions: Multiples of 8 pixels
- Characters: 8x8 pixels each
- Layout: ASCII 32-127 (96 characters)
- Colors: 4 maximum (2bpp)

```
Row 0: SP ! " # $ % & ' ( ) * + , - . /
Row 1: 0 1 2 3 4 5 6 7 8 9 : ; < = > ?
Row 2: @ A B C D E F G H I J K L M N O
Row 3: P Q R S T U V W X Y Z [ \ ] ^ _
Row 4: ` a b c d e f g h i j k l m n o
Row 5: p q r s t u v w x y z { | } ~ DEL
```

### Convert the Image

```bash
font2snes -c assets/myfont.png myfont.h
```

This generates a C header with the tile data and palette.

### Makefile Integration

The Makefile already includes rules for this:

```makefile
FONT_PNG  := assets/myfont.png
FONT_H    := myfont.h

$(FONT_H): $(FONT_PNG)
    @echo "[FONT] $< -> $@"
    @$(FONT2SNES) -c $< $@
```

---

## Build and Run

```bash
cd examples/text/2_custom_font
make clean && make

# Run in emulator
/path/to/Mesen custom_font.sfc
```

---

## VRAM Memory Map

```
$0000-$02A0: Font tiles (42 tiles x 16 bytes = 672 bytes)
$0400-$07FF: BG1 tilemap (32x32 = 1024 words)
```

---

## Files in This Example

| File | Purpose |
|------|---------|
| `main.c` | Complete font and text rendering code |
| `Makefile` | Build configuration + font conversion rules |
| `assets/myfont.png` | Source font image (for font2snes pipeline) |
| `myfont.h` | Generated header (if using font2snes) |

---

## Exercises

### Exercise 1: Add More Characters
Add lowercase letters (a-z) to the font tileset.

**Hint:** Follow the same 2bpp format pattern.

### Exercise 2: Multiple Palettes
Use different palettes for different text (e.g., title in yellow, body in white).

**Hint:** The high byte of tilemap entries contains palette bits.

### Exercise 3: Scrolling Text
Make the text scroll across the screen by modifying the BG1HOFS register.

---

## Key Differences from Hello World

| Aspect | Hello World | Custom Font |
|--------|-------------|-------------|
| Font size | 9 tiles | 42 tiles |
| Characters | H,E,L,O,W,R,D,! | A-Z, 0-9, punctuation |
| Message encoding | Manual array | Pre-encoded arrays |
| Text rendering | Inline loop | Reusable function |

---

## What's Next?

Now that you can display text, let's make things interactive:

**Next:** [Basics - Calculator](../../basics/1_calculator/) - Handle user input

Or explore graphics:

**See also:** [Graphics - Animation](../../graphics/2_animation/)

---

## License

MIT
