# font2snes — font images to text tiles {#tools_font2snes}

The text module ships with a built-in font, so you may never need this tool. But
when you want your *own* typeface — a chunky title font, a themed UI face —
`font2snes` converts a font image into the SNES text tiles the text routines
draw from.

## What goes in, what comes out

**In:** an indexed `.png` holding exactly **96 characters** (ASCII 32–127, so
character 0 is the space), with dimensions that are multiples of 8.

**Out:** either a binary `.pic` of tiles (default), or a C header (`-c`)
declaring `const unsigned char <name>_data[]` you can `#include` and upload at
runtime.

## The flags you will actually use

| Flag | Meaning |
|------|---------|
| `-b N` | bits per pixel: 2 (default) or 4 (4 for Mode 1 BG1/BG2 text) |
| `-c` | emit a C header instead of a binary `.pic` |
| `-v` | verbose |

Note the argument style: font2snes takes **positional** `input output`, unlike
gfx4snes's `-i`.

## Worked examples

```sh
# 2bpp font as a C header (drop-in, no separate asset file):
font2snes -c myfont.png myfont.h

# 4bpp font for a 16-colour text layer:
font2snes -b 4 -c font.png font.h

# Binary tiles instead of a header:
font2snes myfont.png myfont.pic
```

Then load it at runtime and point the text engine at it — a custom font is just
tiles in VRAM, uploaded like any other:

```c
#include "myfont.h"
dmaCopyVram(myfont_data, FONT_VRAM_ADDR, sizeof(myfont_data));
```

## Gotchas worth knowing up front

- **Exactly 96 glyphs, ASCII 32–127.** Character cell 0 must be the space (ASCII
  32). The layout is auto-detected — a 128×48 grid (recommended), a 768×8 strip,
  or any N×M whose cell count is 96.
- **Positional args.** It is `font2snes input output`, not `-i`/`-o`.
- **Optional tool.** font2snes is not wired into the build and no example needs
  it — `textLoadFont` already gives you a usable font. Reach for font2snes only
  when you want a custom typeface; see the notes in the `examples/text/` README.

## Where it fits

A side-branch of the @ref tools pipeline: `font.png` → font2snes → text tiles.
For drawing text with the built-in font (the common case), start from the text
examples such as @ref examples_text_print_string instead.
