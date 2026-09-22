# Text & Fonts Tutorial {#tutorial_text}

This tutorial covers the `text` module — an 8×8 tile font, a 32×32 tilemap
buffer and a cursor, which together are how most OpenSNES examples put words
on screen. Add `text` to `LIB_MODULES` (it pulls in `dma`, `background` and
`console`); add `text4bpp` as well if you want the 4bpp font.

The SNES has no text hardware. A glyph is a tile, a line of text is a row of
tilemap entries, and "printing" is writing those entries. The module owns that
bookkeeping — and, importantly, owns *when* the tilemap reaches VRAM, which is
the part that surprises newcomers.

## Text lands in a RAM buffer, not on the screen

Everything `text*` writes goes into one 2 KB buffer in WRAM:

```
textPrint("HI")      tilemapBuffer[2048]          VRAM tilemap
   │                  32 x 32 entries, 2 bytes        │
   └─ writes entries ─────► each ──────┐              │
                                        │  NMI, next VBlank:
      tilemap_update_flag = 1 ──────────┴──► DMA 2048 bytes ──► ▓▓
```

`tilemapBuffer` is 32×32 entries of 2 bytes = 2048 bytes, and it lives in the
bank `$00` WRAM mirror below `$2000` (the compiler addresses C arrays there —
see @ref tutorial_far_ram for why that band matters).

Every writer in the module — `textPutChar`, `textPrint`, `textPrintAt`,
`textPrintU16`, `textPrintHex`, `textClear`, `textClearRect` — sets
`tilemap_update_flag` on its way out. The NMI handler checks that flag at the
top of each VBlank and, if set, DMAs the whole buffer to VRAM (step 2 of its
execution order, right after the OAM DMA). Nothing you print is visible until
that VBlank happens.

**Coming from PVSnesLib?** `consoleDrawText` wrote to a console buffer you then
flushed. Here the flush is automatic: there is no call to forget. The
counterpart, `textFlush()`, is documented as rarely needed for exactly that
reason — it only sets the flag:

```c
void textFlush(void) {
    tilemap_update_flag = 1;
}
```

Reach for it only when you patched `tilemapBuffer` by hand, outside the `text*`
API. Calling it after a regular `textPrint` is a no-op.

## Setting up in one call

`textModeInit()` is the whole setup for a text-first program. Its Doxygen spells
out exactly what it is equivalent to:

```c
consoleInit();
setMode(BG_MODE0, 0);
setColor(0, 0x0000);
setColor(1, RGB(31, 31, 31));
textInit(TEXT_DEFAULT_TILEMAP_ADDR, TEXT_DEFAULT_FONT_TILE, TEXT_DEFAULT_PALETTE);
textLoadFont(0x0000);
bgSetGfxPtr(0, 0x0000);
bgSetMapPtr(0, 0x3800, BG_MAP_32x32);
setMainScreen(LAYER_BG1);
```

So it assumes: **Mode 0**, text on **BG1**, the 2bpp font at VRAM word
`$0000`, the tilemap at VRAM word `$3800`, palette slot 0, backdrop black and
colour 1 white. That is all of @ref examples_text_print_string —

```c
int main(void) {
    textModeInit();
    setColor(0, RGB(0, 0, 10));

    textPrintAt(8, 14, "TEXT MODULE TEST");

    WaitForVBlank();
    setScreenOn();

    while (1) {
        WaitForVBlank();
    }
}
```

Two consequences of that equivalence list are worth internalising, because they
bite in every example that does more than print:

- It ends with `setMainScreen(LAYER_BG1)`, so **sprites are off**. Call
  `setMainScreen(LAYER_BG1 | LAYER_OBJ)` afterwards if you want both, as
  @ref examples_basics_game_skeleton does.
- It puts the font at VRAM `$0000`, so **your sprite or BG tiles must go
  somewhere else**. `game_skeleton` and @ref examples_basics_aim_target both
  park sprite graphics at `$4000` for this reason:

  ```c
  #define SPR_VRAM   0x4000   /**< sprite tiles (text font is at $0000) */
  ```

## Setting up by hand

When text is a HUD over a game rather than the whole program, do the two steps
yourself:

```c
void textInit(u16 tilemap_addr, u16 font_tile, u8 palette);
void textLoadFont(u16 vram_addr);      /* 2bpp, 1536 bytes */
void textLoadFont4bpp(u16 vram_addr);  /* 4bpp, needs `text4bpp` in LIB_MODULES */
```

- `tilemap_addr` is a VRAM **word** address — the same unit as `bgSetMapPtr()`
  and the rest of the SDK. (Before v0.23.0 it was a byte address; code that
  passed `W * 2` now passes `W`.)
- `font_tile` is the tile number of the first glyph **relative to the layer's
  tile base**, not an absolute VRAM address.
- `palette` is a slot 0–7; `textInit` masks it with `& 0x07`.

`textInit` also resets the cursor to (0, 0), fills the buffer with the space
glyph and performs an **immediate** DMA to VRAM — not a flagged one. That is a
raw VRAM write, so like `textLoadFont`, it belongs in forced blank, before
`setScreenOn()`.

Wiring the BG layer is still yours. @ref examples_games_rpg puts 2bpp text on
BG3 in Mode 1, in front of the map:

```c
/* BG3 text overlay */
textInit(VRAM_TEXT_MAP, 0, 4);
text_config.priority = 1;
textLoadFont(VRAM_FONT);
bgSetGfxPtr(2, VRAM_FONT);
bgSetMapPtr(2, VRAM_TEXT_MAP, SC_32x32);
setColor(4 * 4 + 1, RGB(31, 31, 31));
```

Note `text_config.priority` — the `TextConfig` struct is public (`extern
TextConfig text_config`), and its `priority` field goes into bit 13 of every
tilemap entry the module builds from then on. Setting it right after `textInit`
is the supported way to put text in front of an opaque background.

@ref examples_maps_dynamic_map shows the 4bpp case, and the `font_tile`
arithmetic that goes with a font *not* placed at the layer's tile base:

```c
#define VRAM_BG2_GFX    0x2000
#define VRAM_FONT       0x3000
/* The font is loaded at VRAM_FONT but the base is VRAM_BG2_GFX, so
 * the first font tile has index (VRAM_FONT - VRAM_BG2_GFX) / 16 = 0x100. */
#define FONT_TILE_OFFSET 0x100

bgSetGfxPtr(1, VRAM_BG2_GFX);
bgSetMapPtr(1, VRAM_BG2_MAP, SC_32x32);
textLoadFont4bpp(VRAM_FONT);
textInit(VRAM_BG2_MAP, FONT_TILE_OFFSET, 1);
```

Get that offset wrong and you get glyphs from elsewhere in VRAM — legible
garbage, which is the confusing kind.

## Writing glyphs and moving the cursor

```c
void textPutChar(char c);
void textPrint(const char *str);
void textPrintAt(u8 x, u8 y, const char *str);
inline void textSetPos(u8 x, u8 y);     /* in text.h, inlined */
u8   textGetX(void);
u8   textGetY(void);
```

`textPrint` is a `textPutChar` loop; `textPrintAt` is `textSetPos` followed by
`textPrint`. So all the behaviour lives in `textPutChar`:

| Input | What happens |
|---|---|
| `'\n'` | column → 0, row → row + 1 |
| `'\r'` | column → 0, row unchanged |
| ASCII 32–127 | glyph written at the cursor, column → column + 1 |
| anything else | the **space** glyph is written (`font_tile`), column advances |

Two wraps, and both are guards rather than errors:

- **End of a line.** Past column 31 the cursor returns to column 0 on the next
  row. There is no word wrapping — a long string simply continues one row down.
- **End of the last row.** Past row 31 the cursor returns to **row 0**. This is
  the bound that keeps output inside the 2048-byte buffer, and it is pinned by
  the library fixture: `devtools/libtests/main.c` prints 40 rows of an
  eight-glyph string plus a newline each, then `test_libtest.py` asserts
  `textGetY() == 8` (40 wrapping to 40 − 32) and `text_config.map_width == 32`.
  The second assert is the real point — before the wrap existed, row 32 wrote
  past `tilemapBuffer[2048]` and the first casualty was `text_config` itself.

`textSetPos` masks rather than clamps: `x & 31` and `y & 31`. An x of 40 becomes
column 8, not column 31 and not an error.

One geometry note: the buffer is 32 rows but a standard 224-line display shows
only **28** of them (224 / 8). Rows 28–31 exist, are flushed, and are simply
off-screen until you scroll the layer — which is exactly what
@ref examples_text_scroll_message does with `bgSetScroll`.

## Printing numbers

```c
void textPrintU16(u16 value);
void textPrintHex(u16 value, u8 digits);
```

`textPrintU16` prints 1–5 decimal digits at the cursor: most-significant first,
no leading zeros, no padding, and a bare `0` for zero. `textPrintHex` prints
`digits` uppercase hex digits (clamped to 4 internally), with **no** `0x`
prefix — @ref examples_basics_aim_target adds its own:

```c
textSetPos(9, 20); textPrint("0X");
textPrintHex(angle, 2);
```

Because neither function pads, a shrinking number smears: print 100 then 99 and
you are looking at `990`. The idiom across the corpus is to blank the field
first, as `game_skeleton` does:

```c
/** @brief Refresh the SCORE number (clear the field first so it never smears). */
static void draw_score(void) {
    textPrintAt(8, 1, "     ");
    textSetPos(8, 1);
    textPrintU16(score);
}
```

## Erasing

```c
void textClear(void);
void textClearRect(u8 x, u8 y, u8 w, u8 h);
```

`textClear()` fills the whole 32×32 buffer with the space glyph via an assembly
fill loop (~14 000 cycles, against ~238 000 for the compiled-C equivalent it
replaced). It does **not** move the cursor — set the position yourself
afterwards.

`textClearRect()` blanks a rectangle and clamps hard: a start position outside
the buffer returns immediately, and a width or height that would run off the
edge is truncated to fit. You cannot make it write outside the buffer.

@ref examples_basics_scene_stack shows the typical rhythm — `textClear()` when
entering a scene, then the static labels, then per-frame updates into fixed
fields.

## What a text update costs

The flush is **all-or-nothing**: if any text writer ran since the last VBlank,
the NMI handler DMAs the full **2048 bytes**, whether you changed one glyph or
the whole screen. Against the roughly 4 KB of practical VBlank DMA budget
(@ref tutorial_dma) that is about half the frame's transfer capacity spent on
text.

Two corollaries:

- **Many writes, one DMA.** The flag coalesces — twenty `textPrintAt` calls in a
  frame still cost one 2048-byte transfer. Batch freely.
- **Don't reformat text you did not change.** The formatting loops themselves
  are not cheap either. `aim_target` learned this the measured way:

  ```c
  /* Skip the recompute + text refresh when the target hasn't
   * moved. The text writers are heavy (formatting loops + DMA
   * flush flag) and re-running them every frame burned half the
   * frame budget on idle frames — caught by the lag-detection
   * phase before this guard was added. */
  if (target_x == prev_target_x && target_y == prev_target_y) return;
  ```

If your HUD only changes on a score or timer tick, guard it with a dirty flag
and the text DMA disappears from most frames entirely. @ref craft_frame_budget
puts this in the context of the rest of the frame.

## Fonts, bit depth and palettes

The module ships with a built-in font, so the common case needs no asset at all.
It is 96 glyphs covering **ASCII 32–127**, in tile order (index = ASCII − 32),
and it lives in ROM as `opensnes_font_2bpp` in a SUPERFREE section:

| Call | Format | Size | Module |
|---|---|---|---|
| `textLoadFont(addr)` | 2bpp, 16 bytes/tile | 1536 bytes | `text` |
| `textLoadFont4bpp(addr)` | 4bpp, 32 bytes/tile, bitplanes 2–3 zero | 3072 bytes | `text4bpp` |

Use the 4bpp variant when the text layer is a 4bpp BG — BG1 or BG2 in Mode 1 —
and remember to add `text4bpp` to `LIB_MODULES`, or the link fails on
`textLoadFont4bpp`. The 4bpp font is a separate module precisely so the 3 KB
does not land in every ROM.

Both loaders write VRAM directly through DMA, so **call them during forced
blank**, before `setScreenOn()`.

### Which colour is the text?

The built-in face is effectively 1-bit: bitplane 1 of every tile is zero. Glyph
pixels are therefore **colour index 1** of the chosen palette slot, and colour 0
is the transparent background. To colour your text, write colour 1 of its slot:

| Depth | Colours per slot | Text colour for slot `P` | Example |
|---|---|---|---|
| 2bpp | 4 | `setColor(P * 4 + 1, c)` | `rpg`, slot 4 → `setColor(17, …)` |
| 4bpp | 16 | `setColor(P * 16 + 1, c)` | `dynamic_map`, slot 1 → `setColor(17, …)` |

`textModeInit()` does the 2bpp slot-0 case for you: `setColor(1, RGB(31, 31,
31))`, white on a black backdrop.

### Your own typeface

A custom font is just different tiles at the same VRAM address. Convert an
indexed PNG of exactly 96 glyphs (ASCII 32–127, space first) with
@ref tools_font2snes, upload it yourself, and point `textInit` at it:

```c
#include "myfont.h"
dmaCopyVram(myfont_data, FONT_VRAM_ADDR, sizeof(myfont_data));
textInit(TEXT_DEFAULT_TILEMAP_ADDR, 0, 0);
```

font2snes is optional and not wired into the build — no example needs it,
because `textLoadFont` already gives you a usable face.

## Mistakes the module will not save you from

The code guards the ones it can: the cursor wraps instead of overflowing the
buffer, `textClearRect` clamps to the edges, `textPrintHex` clamps `digits` to
4, `textSetPos` masks its arguments, and a character outside ASCII 32–127
becomes a space rather than a wild tile index. These are the ones left to you.

### 🔴 Loading the font or calling `textInit` with the screen on

Both write VRAM directly — `textLoadFont` sets VMADD and fires a DMA,
`textInit` ends with an immediate tilemap flush. Outside forced blank or VBlank
the PPU silently drops those writes and you get a screen of garbage tiles with
no error anywhere. Init first, `setScreenOn()` last.

### 🔴 Sprite or BG tiles at VRAM `$0000` after `textModeInit()`

`textModeInit` loads the font at word `$0000`, occupying 1536 bytes. Anything
else you upload there overwrites glyphs — usually visible as a few letters
turning into fragments of your sprite. Put other graphics at `$4000` (or
anywhere clear) as the examples do.

### 🟡 Expecting text before the first VBlank

Printing sets a flag; the NMI does the work. A program that prints and then
spins without ever reaching a VBlank shows nothing. The canonical opening is
`WaitForVBlank()` then `setScreenOn()`, then a loop whose first statement is
`WaitForVBlank()`.

### 🟡 Sprites vanishing right after `textModeInit()`

Its last act is `setMainScreen(LAYER_BG1)`. Re-call `setMainScreen` with
`LAYER_BG1 | LAYER_OBJ` after it, not before.

### 🟡 Numbers smearing

`textPrintU16` and `textPrintHex` do not pad. Blank the field before rewriting
it (see the `draw_score` idiom above).

### 🟡 Passing a byte address to `textInit`

`tilemap_addr` has been a VRAM **word** address since v0.23.0. Ported code that
passes `0x7000` for a tilemap at word `$3800` puts the DMA target 16 KB past
where the BG reads its map — a blank layer, no diagnostic.

### 🟡 Assuming the tilemap can be 64 wide

`TextConfig` has a `map_width` field documented as 32 or 64, but `textInit`
always sets it to 32, the buffer is sized for 32×32, and the NMI flush transfers
a fixed 2048 bytes. Treat `map_width` as read-only; a 64-wide text layer is not
something the module supports today.

## See also

- @ref text.h "Text API reference" — the 14 public functions and `TextConfig`.
- `lib/source/text.c` and `lib/source/text.asm` — the buffer, the cursor, and
  the font data.
- @ref tutorial_dma — the VBlank DMA budget the 2048-byte flush is spent from.
- @ref craft_frame_budget — where text sits among everything else a frame does.
- @ref tutorial_panel — bordered boxes to put text *in*; the box is one layer,
  the text another.
- @ref tools_font2snes — converting your own typeface.
- @ref examples_text_print_string — the minimum program.
- @ref examples_text_scroll_message — the text layer scrolled like any BG.
- @ref examples_fundamentals_text_glyphs — what the module hides: a glyph as
  raw 2bpp bitplanes plus a tilemap entry.
- @ref examples_games_rpg and @ref examples_maps_dynamic_map — the explicit
  `textInit` path, 2bpp and 4bpp.
