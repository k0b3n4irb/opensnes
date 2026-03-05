# Text Examples

The SNES has no built-in text rendering. Text is displayed the same way as any
other graphics: font characters are stored as tiles in VRAM, and a tilemap
references those tiles to spell out words on screen.

These examples teach the tile/tilemap fundamentals that underpin all SNES graphics.

## Examples

| Example | Difficulty | Description |
|---------|------------|-------------|
| [hello_world](hello_world/) | Beginner | Your first SNES ROM -- display text on a background layer |
| [text_test](text_test/) | Beginner | Text positioning, formatting, and consoleDrawText usage |

## Key Concepts

### How Text Works on SNES

```
Font Image (PNG) --> gfx4snes/font2snes --> Tile Data (VRAM)
                                        --> Tilemap (VRAM)
                                                |
                                        PPU renders screen
```

Each character is an 8x8 pixel tile. A font is a collection of tiles covering
the ASCII range. The tilemap is a grid of tile numbers that defines which
character appears at each screen position.

### 2BPP Tile Format

"2bpp" means 2 bits per pixel = 4 possible colors (indices 0-3).
Each 8x8 tile is 16 bytes: 8 bytes for bitplane 0, 8 bytes for bitplane 1.

### VRAM Layout for Text

```
$0000-$0FFF: Tile data (font graphics)
$1000-$17FF: BG1 tilemap (32x32 = 2KB)
```

### Mode 0 Palette

In Mode 0, each background layer gets 4 colors:
- BG1: Colors 0-3
- BG2: Colors 4-7
- BG3: Colors 8-11
- BG4: Colors 12-15

---

Start with **hello_world** to get your first ROM running, then use **text_test**
to explore text positioning and formatting.
