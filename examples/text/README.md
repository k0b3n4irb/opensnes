# Topic: Text Display

Learn how the SNES displays text and graphics using tiles and tilemaps.

## What You'll Learn

- How VRAM stores tile graphics
- How tilemaps reference tiles to build screens
- The 2bpp tile format (4 colors)
- Using the font2snes tool

## SNES Text Rendering Explained

The SNES doesn't have a "print text" function. Instead, text is displayed like any other graphics:

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  Font Image  │ ──▶ │  Tile Data   │ ──▶ │   Tilemap    │
│  (PNG file)  │     │   (VRAM)     │     │   (VRAM)     │
└──────────────┘     └──────────────┘     └──────────────┘
                            │                    │
                            ▼                    ▼
                     ┌──────────────────────────────┐
                     │        PPU renders screen    │
                     └──────────────────────────────┘
```

### Step 1: Font as Tiles

Each character is an 8×8 pixel **tile**. A font is just a collection of tiles:

```
┌─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┬─┐
│A│B│C│D│E│F│G│H│I│J│K│L│M│N│O│P│  Tile 0-15
├─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┼─┤
│Q│R│S│T│U│V│W│X│Y│Z│0│1│2│3│4│5│  Tile 16-31
└─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┘
```

### Step 2: Tilemap

A **tilemap** is a grid of tile numbers that defines what appears on screen:

```
Tilemap (32×32 tiles = 256×256 pixels):
┌────┬────┬────┬────┬────┬────┬────┬────┐
│  0 │  7 │ 11 │ 11 │ 14 │  0 │ 22 │ 14 │ ← "HELLO WO"
├────┼────┼────┼────┼────┼────┼────┼────┤    (tile numbers)
│ 17 │ 11 │  3 │  0 │  0 │  0 │  0 │  0 │ ← "RLD"
└────┴────┴────┴────┴────┴────┴────┴────┘

Each entry = 2 bytes: tile number + attributes
```

## 2BPP Tile Format

"2bpp" means **2 bits per pixel** = 4 possible colors (0-3).

Each 8×8 tile = 16 bytes:
- Bytes 0-7: Bitplane 0 (bit 0 of each pixel)
- Bytes 8-15: Bitplane 1 (bit 1 of each pixel)

```
Example: Letter "H"
Pixels:           Bitplane 0:    Bitplane 1:
. # . . . . # .   01000010       00000000
. # . . . . # .   01000010       00000000
. # . . . . # .   01000010       00000000
. # # # # # # .   01111110       00000000
. # . . . . # .   01000010       00000000
. # . . . . # .   01000010       00000000
. # . . . . # .   01000010       00000000
. . . . . . . .   00000000       00000000
```

Color index = (bitplane1[bit] << 1) | bitplane0[bit]

---

## Lessons

### [1. Hello World](hello_world/)
**Difficulty:** ⭐ Beginner

Your first SNES ROM! Display "HELLO WORLD" on screen.

**Concepts:**
- VRAM and tilemap basics
- Background mode 0
- Inline font data

**Key Registers:**
- `$2105` BGMODE - Set graphics mode
- `$2115-2119` VRAM access
- `$212C` TM - Enable layers

---

### [2. Custom Font](custom_font/)
**Difficulty:** ⭐⭐ Beginner+

Create your own font and display multiple lines of text.

**Concepts:**
- Asset pipeline (PNG → SNES)
- The `font2snes` tool
- Full ASCII character set (32-127)

**New Skills:**
- Using conversion tools
- Working with assets/

---

## Quick Reference

### VRAM Layout for Text

```
$0000-$0FFF: Tile data (font graphics)
$1000-$17FF: BG1 tilemap (32×32 = 2KB)
```

### Mode 0 Palette

In Mode 0, each background uses 4 colors:
- BG1: Colors 0-3
- BG2: Colors 4-7
- BG3: Colors 8-11
- BG4: Colors 12-15

---

**Next Topic:** [Graphics](../graphics/) - Learn to display sprites →
