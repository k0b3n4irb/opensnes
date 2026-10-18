# Upgrading from 0.48 to 0.49 {#upgrading}

The releases up to 0.48 deprecate; 0.49 removes, so that 1.0 has nothing
left to remove. Every name below built with a warning in 0.48 (the clang
pre-pass reported each use; without clang the build ran the same scan in
Python on each source it compiled) and is gone at 0.49: a project that
names one no longer compiles, and `opensnes upgrade <folder>` reads a
whole project against the list and says what to use. This is the API 1.0
is expected to freeze (`docs/STABILITY.md`).

**State at 0.49.0 (2026-10-10; the removals date from 2026-10-05):** the constants of
section 3, the `OAM_SET_GFX_BANK` macro and the twenty-six renamed
functions of section 2 are gone from the headers, `opensnes upgrade` reports
each use from `make/removed_api.txt`, `hdmaEnable()` / `hdmaDisable()`
take a channel number and `dmaTransfer()` a far pointer (section 1). No
`OPENSNES_DEPRECATED` declaration is left.

Two things on this page are not removals but **changes of meaning**: read
them first. Everything else is a rename where the old and new name do the
same thing.

## 1. Two calls that change meaning

### `hdmaEnable(x)` and `hdmaDisable(x)` take a channel number at 0.49

Until 0.48 they took a **bit mask** (`1 << channel`), like nothing else in
`hdma.h`. Since 0.48 that meaning has its own names, `hdmaEnableMask()` and
`hdmaDisableMask()`, and in 0.48 the two short names warn. At 0.49 (on
`develop` since 2026-10-05) they take a **channel number** 0-7, like the
other twenty functions of the header. A call left as `hdmaEnable(0x40)`
compiles and means something else; values above 7 are refused (nothing is
enabled), so `0x40`, `0x0F`, `0xFF` and `1 << 6` fail visibly, but the masks
1, 2 and 4 silently become channels 1, 2 and 4 instead of 0, 1 and 2.

```c
hdmaEnable(1 << HDMA_CHANNEL_6);      /* 0.48: mask. 0.49: refused (64 > 7) */
hdmaEnableMask(1 << HDMA_CHANNEL_6);  /* 0.48+: the mask, by its name     */
hdmaEnable(HDMA_CHANNEL_6);           /* 0.49: the channel                 */
```

### `dmaTransfer()` takes the source as one far pointer at 0.49

Until 0.48 the generic transfer took the bank and the 16-bit address as
two arguments; since 0.49 the source is a `const u8 *`, whose bank byte the
function reads, like every `dmaCopy*` helper. A six-argument call no longer
compiles (one argument too many), so nothing changes silently.

```c
dmaTransfer(1, 0x00, bank, addr, 0x22, 4);   /* 0.48 */
dmaTransfer(1, 0x00, table, 0x22, 4);        /* 0.49: the far pointer */
```

### `mode7SetScale(0x0100)` is 1:1 since 0.47

Before 0.47 the helpers divided the scale by two: `0x0100` magnified twice
and `0x0200` was 1:1. A project that passed `0x0200` for a 1:1 view sees
its plane shrink; pass `0x0100`. `mode7Transform(deg, 100)` is 1:1 too.
No warning can catch this one: search your sources for `mode7SetScale` and
`mode7Transform`.

## 2. Renamed functions (same behaviour)

| Removed at 0.49 | Use instead | Header | Note |
|---|---|---|---|
| `rand()` | `rngNext()` | `console.h` | not libc's `rand`: 1-65535, a 16-bit LFSR |
| `srand(s)` | `rngSeed(s)` | `console.h` | |
| `getRegion()` | `isPAL()` | `console.h` | the same value, 0 NTSC / 1 PAL |
| `consoleInitEx(o)` | `consoleInit()` | `console.h` | the argument was always ignored |
| `profileGetFrameCount()` | `getFrameCount()` | `profile.h` | the same counter |
| `padRaw(p)` | `padHeld(p)` | `input.h` | never the raw register. One difference: `padHeld` answers 0 for an unplugged port's `$FFFF`, `padRaw` returned the `$FFFF` |
| `scopeButtonsDown()` | `scopeButtonsHeld()` | `input.h` | "currently down", like `padHeld` |
| `nmiSetBank(f, b)` | `nmiSet(f)` | `interrupt.h` | the bank comes from the pointer. `nmiSet(NULL)` disarms; `nmiSetBank(NULL, b)` installed a jump to nothing |
| `irqSetBank(f, b)` | `irqSet(f)` | `interrupt.h` | idem |
| `dmaCopyVramBank(s, b, …)` | `dmaCopyVram(s, …)` | `dma.h` | idem |
| `dmaCopyCGramBank(s, b, …)` | `dmaCopyCGram(s, …)` | `dma.h` | idem |
| `hdmaSetupBank(c, m, r, t, b)` | `hdmaSetup(c, m, r, t)` | `hdma.h` | idem |
| `OAM_SET_GFX_BANK(id, p, b)` | `OAM_SET_GFX(id, p)` | `sprite.h` | idem (macro) |
| `colorMathEnable(l)` | `colorMathSetLayers(l)` | `colormath.h` | it replaces the set, it never added to it |
| `mosaicEnable(m)` | `mosaicSetLayers(m)` | `mosaic.h` | idem |
| `LzssDecodeVram(s, a)` | `lzssDecodeVram(s, a)` | `lzss.h` | lower-case `l` |
| `sa1Init()` | `sa1IsReady()` | `sa1.h` | crt0 boots the SA-1; this only read its status |
| `dsp1Present()` | `dsp1IsPresent()` | `dsp1.h` | returns 0 / 1 |
| `dsp1Parameter(fx, fy, fz, lfe, les, aas, azs)` | `dsp1SetCamera(&cam)` | `dsp1.h` | the seven arguments are the fields of `Dsp1Camera`, in order |
| `ease_in_quad(t)` / `ease_out_quad(t)` | `easeInQuad(t)` / `easeOutQuad(t)` | `math.h` | |
| `mode7SetPivot(x, y)` | `mode7SetCenter(x, y)` | `mode7.h` | the same two registers, without the 0-255 limit |
| `oamDrawMeta(id, x, y, m, tile, pal, size)` | `oamDrawMetasprite(id, x, y, m, &style, 0)` | `sprite.h` | `tile`, `pal`, `size` are `MetaspriteStyle` fields |
| `oamDrawMetaFlip(id, x, y, m, tile, pal, size, fx, fy, w, h)` | `oamDrawMetasprite(id, x, y, m, &style, flip)` | `sprite.h` | `style.pieceSize` replaces the old function's assumption that a piece is 16 pixels when large and 8 when small (wrong for 32-pixel pieces) |
| `audioUpdate()` | remove the call | `audio.h` | it did nothing |
| `snesmodSetSoundTable(t)` | remove the call | `snesmod.h` | no SDK call starts a stream; the table was never read |
| `snesmodAllocateSoundRegion(n)` | remove the call | `snesmod.h` | idem; it did take memory from the module |

## 3. Renamed constants (same values)

| Removed at 0.49 | Use instead | Header |
|---|---|---|
| `BGMODE_MODE0`, `BGMODE_MODE1`, `BGMODE_MODE2`, `BGMODE_MODE3`, `BGMODE_MODE7` | `BG_MODE0` … `BG_MODE7` | `registers.h` → `video.h` |
| `WINDOW_BG1` … `WINDOW_BG4`, `WINDOW_OBJ` | `LAYER_BG1` … `LAYER_BG4`, `LAYER_OBJ` | `window.h` → `video.h` |
| `COLORMATH_BG1` … `COLORMATH_BG4`, `COLORMATH_OBJ` | `LAYER_BG1` … `LAYER_OBJ` | `colormath.h` → `video.h` |
| `MOSAIC_BG1` … `MOSAIC_BG4` | `LAYER_BG1` … `LAYER_BG4` | `mosaic.h` → `video.h` |

What only one module has keeps its name: `WINDOW_MATH`, `WINDOW_ALL`,
`WINDOW_ALL_BG`, `COLORMATH_BACKDROP`, `COLORMATH_ALL`, `MOSAIC_BG_ALL`,
and the `TM_*` register bit names of `registers.h`.

## 4. Other changes a 0.48 project may notice

- **Struct assignment works** (`a = b;`) since 0.48 for objects in bank $00
  or in ROM; it was refused by accident. A copy from or to a `FAR` struct
  is refused: copy field by field. **Struct returns by value are refused**
  (they compiled to a copy of zeros); return through a pointer argument.
- **`FAR` objects and bit-fields**: `far++`, `far--`, a bit-field of a
  `FAR` object or of const data read through a pointer, and `= {0}` on a
  `u32` array or a struct with an `s32` were miscompiled before 0.48. If a
  0.47 project used them, its behaviour changes — to the right one.
- **`sramSave()`, `sramLoad()`, `sramClear()` on a Super FX cartridge**
  (`USE_SRAM` with `USE_SUPERFX`, new in 0.48) address the GSU's own RAM
  from offset 0, where the framebuffers are: use the `Offset` variants and
  a region of your own.

## 5. Checking a project

Build it: a name that was removed fails to compile, and the build prints
its replacement under the error (`opensnes upgrade <folder>` lists every
one at once). Then search for the two calls that change meaning:

```sh
grep -rnE 'hdma(Enable|Disable)\(|mode7SetScale\(|mode7Transform\(' src/
```

`opensnes upgrade <folder>` lists every removed name with its
replacement, and every `hdmaEnable` / `hdmaDisable` / `dmaTransfer` /
`mode7SetScale` / `mode7Transform` call, one line per hit (the names are
`make/removed_api.txt` and `make/changed_api.txt` in the SDK; nothing to
install — in the repository, `make check-upgrade SRC=<folder>` is the same
command). Exit 0 when nothing is found. A source that fails to compile
gets the removed names it uses, with their replacements, right under the
compiler's error.

## After 0.50: the earlier converters are retired

`gfx4snes`, `smconv`, `wav2brr`, `tmx2snes`, `aseprite2snes`, `font2snes`,
`img2snes` and `palplan` are no longer in `bin/` or in the zip. Their code
is linked into the `opensnes-*` tools, which write the same bytes.

- **Music** (`USE_SNESMOD`, `SOUNDBANK_SRC`): nothing to change. The build
  converts the modules with `opensnes-music`, to the same soundbank.
- **`GFXSRC`**: the build stops with a message. Put a settings file beside
  each picture instead (`res/hero.png.toml` with `tool = "opensnes-sprite"`
  for a sprite sheet, `"opensnes-tileset"` for a background);
  @ref tools_legacy maps the gfx4snes options.
- **A `.wav` whose `.brr` an assembly file `.incbin`s**: the automatic
  rule is gone. A `res/sound.wav.toml` with `tool = "opensnes-sample"`
  converts it, or commit the `.brr`.
- **A script that called one of these commands**: @ref tools_legacy gives
  the replacing command, option by option.


## After 0.50: what else a 0.50 project may meet

- **`opensnes-sprite sheet --pack`** is gone, and so is `pack = true` in a
  `[sheet]` table: the tool refuses both. Packed pixels are the Mode 7
  background format, which sprites cannot read; `opensnes-tileset convert
  --pack` (or `--mode 7`) still writes it.
- **A variable-length array is refused.** `u16 a[n];` with `n` known only
  at run time — a `const` variable is not a constant expression in C — used
  to compile to a frame that was too small. The compiler now stops and says
  so; give the length as an `enum` or a `#define`.
- **`opensnes-music bank` refuses a module the SPC700's RAM cannot hold**
  (exit 1, nothing written) where it used to warn and write the soundbank.
  The figures `inspect` and `--json` print (`spc_ram_bytes`,
  `spc_ram_free`) are now what the driver loads, against the room its echo
  leaves (@ref tools_opensnes_music).
- **A ROM the post-link check refuses is deleted.** The build used to stop
  with the error but leave the `.sfc`, which a second `make` then took as
  up to date. Run `opensnes-rom check` on the `.sym`, which stays.
- **An SA-1 ROM whose `SA1Start` is outside bank $00 is refused**: the
  SA-1 reset vector is 16-bit. Declare the boot section
  `.SECTION ".sa1_boot" BANK 0 SLOT "ROM" SEMIFREE`, as
  `templates/sa1_boot.asm` does (@ref tutorial_sa1).
