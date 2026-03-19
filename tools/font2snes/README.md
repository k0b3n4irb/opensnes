# font2snes

Converts a PNG font image to SNES tile format (2bpp or 4bpp).

## Usage

```bash
font2snes [options] input.png output
```

The input PNG must contain exactly 96 characters (ASCII 32-127) as 8x8 tiles.

## Supported Layouts

The tool auto-detects the image layout:

| Layout | Dimensions | Description |
|--------|-----------|-------------|
| **Grid** (recommended) | 128x48 px | 16 columns x 6 rows of 8x8 characters |
| **Strip** | 768x8 px | 96 characters in a single row |
| **Custom** | NxM where N*M/64 = 96 | Any arrangement that fits 96 tiles |

## Examples

### Binary output (2bpp, for Mode 0 or BG3)

```bash
font2snes myfont.png myfont.pic
```

### C header output

```bash
font2snes -c myfont.png myfont.h
```

Generates a C array: `const unsigned char myfont_data[] = { ... };`

### 4bpp output (for Mode 1 BG1/BG2)

```bash
font2snes -b 4 myfont.png myfont.pic
```

### 4bpp as C header

```bash
font2snes -b 4 -c myfont.png myfont.h
```

## Options

| Flag | Description |
|------|-------------|
| `-b N` | Bits per pixel: `2` (default) or `4` |
| `-c` | Output as C header instead of binary |
| `-v` | Verbose output |
| `-h` | Show help |
| `-V` | Show version |

## Requirements

- Input must be an indexed PNG
- Dimensions must be multiples of 8
- Must contain exactly 96 characters
- Character 0 = ASCII 32 (space), character 95 = ASCII 127 (DEL)

## Using with the Text Module

```c
#include <snes/text.h>

textInit();
textLoadFont(0x0000);    // 2bpp: loads built-in font
// OR load your custom font:
// dmaCopyVram(myfont_data, 0x0000, sizeof(myfont_data));
```

For 4bpp backgrounds, use `textLoadFont4bpp()` and add `text4bpp` to `LIB_MODULES`.
