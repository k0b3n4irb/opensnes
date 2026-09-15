# Migrating from PVSnesLib {#migrating_pvsneslib}

OpenSNES and [PVSnesLib](https://github.com/alekmaul/pvsneslib) target the same
machine and share ancestry, so most of a port is mechanical. What bites is the
handful of places where the two differ *silently* — code that compiles, links,
boots, and then draws the wrong thing. This page is those places, in the order
they usually hurt.

If you are reading this with a PVSnesLib project open, work top to bottom: the
build system first, then the five traps, then the API table.

## What is the same

Almost all of the surface. Both SDKs give you `consoleInit`, `setMode`,
`bgSetGfxPtr`, `bgSetMapPtr`, `oamSet`, `WaitForVBlank`, the same PPU register
model, the same VRAM layout decisions and the same `.pic` / `.pal` / `.map`
asset trio produced by a gfx4snes-shaped converter. A simple sprite demo often
ports by changing the Makefile and nothing else.

## What is different, in one table

| Area | PVSnesLib | OpenSNES | Where it bites |
|---|---|---|---|
| Compiler | tcc816 | cc65816 (cproc + QBE) | Argument push order, refused constructs |
| Argument push | right to left | **left to right** | Hand-written ASM callees read the wrong slots |
| `int` | 2 bytes | 2 bytes | Same |
| `long` | 4 bytes | 4 bytes | Same |
| Modules | linked wholesale | **opt-in** `LIB_MODULES` | Link errors until you list what you use |
| Text output | `consoleDrawText` | `textPrintAt` + `textFlush` | Nothing appears without the flush |
| Vertical scroll | raw value to the register | lib subtracts 1 for you | Everything is one line off |
| Const data | wherever it lands | asset banks, far reads | A cast away from `const` reads garbage |
| Bulk RAM | plain globals | plain under `$2000`, else `FAR` | A big buffer silently wrong-banked |

## Step 1 — the Makefile

An OpenSNES example Makefile is short, and the module list is the part that is
new:

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := game.sfc
ROM_NAME := MY GAME
USE_LIB  := 1
LIB_MODULES := console sprite dma input text
CSRC := main.c
include $(OPENSNES)/make/common.mk
```

`LIB_MODULES` is the one real change of habit. OpenSNES links only the modules
you name, so an undefined reference at link time means "add the module", not
"the function is missing". Dependencies resolve themselves: listing `text`
pulls in `dma`, `background` and `console`.

Set `USE_SRAM := 1` for battery saves, `USE_SA1 := 1` or `USE_SUPERFX := 1` for
the coprocessors, `USE_HIROM := 1` for a HiROM map.

## Step 2 — the five traps

### Trap 1: arguments are pushed left to right

This is the one that breaks ported assembly. cc65816 pushes the **first**
argument first, so after the usual `php` / `phb` prologue the first argument is
at `6,s`, the second at `8,s`, and so on — the mirror image of PVSnesLib.

Pure C ports never notice. Any `.asm` file you bring over that reads its
arguments off the stack must have its offsets rewritten. `compiler/ABI.md` is
the reference, and `make lint-asm-abi` checks every hand-written library
routine against its C prototype, so a mistake fails the build instead of
corrupting a register at runtime.

### Trap 2: `setScreenOn()` is always last

PVSnesLib code often turns the screen on early and loads VRAM afterwards.
OpenSNES expects the whole setup to happen during forced blank:

```
consoleInit() → setMode() → palettes → tiles → tilemap → BG pointers
→ sprites → text → setMainScreen() → setScreenOn()
```

VRAM writes during active display are **silently dropped** by the PPU, so an
early `setScreenOn()` shows up as missing or garbage tiles on the first frames,
with no error anywhere.

### Trap 3: sprite palettes start at CGRAM 128

Background palettes live at CGRAM 0-127, sprite palettes at 128-255. A sprite
palette uploaded to offset 0 gives you a sprite drawn in the background's
colours — a bug that looks like an art problem.

### Trap 4: vertical scroll already has the -1

The PPU never outputs scanline 0, so a raw `y` written to `BGnVOFS` shows row
`y+1`. PVSnesLib writes the raw value and leaves the correction to you;
OpenSNES does it for you in `bgSetScroll`, the map module and `mode7SetScroll`.

So a port that carefully passed `y - 1` now scrolls one line too far. Pass the
row you actually want. The exception is data that reaches the register without
the library — an HDMA table of `BGnVOFS` values still needs its own -1.

### Trap 5: const data is far, and casting `const` away breaks it

Since the bank-$00 flip, C const data (arrays, string literals, const structs)
is placed in the asset banks and every C read of it is a far read. That is
invisible and free — until you cast the `const` away and read through a plain
pointer, which reads bank $00 and returns garbage. The link fails on it
(`devtools/check_bank_reads.py`), so you get told; the fix is to keep the
`const`, or to pass the data to a library function as a far pointer.

The RAM twin: plain C globals must sit below `$2000`. Anything bigger — a
tilemap buffer, an entity pool, an HDMA table — is declared `FAR` and lives in
`$7E:2000-$FFFF`. See @ref tutorial_far_ram.

## Step 3 — constructs cc65816 refuses

These stop the build with a message naming the feature. None of them is
miscompiled silently, and each has a mechanical workaround:

| Refused | Workaround |
|---|---|
| Struct passed or returned **by value** | Pass a pointer to it |
| Struct assignment by value (`a = b`) | Copy the fields, or `memcpy` |
| Variadic functions (`...`) | Fixed-arity wrappers |
| Inline assembly inside C | A separate `.asm` file with a C prototype |

`printf` is not in the core library by design. Use `textPrint`, `textPrintU16`
and `textPrintHex`, or the debug channel (@ref tutorial_debugging) when you
need output to the host rather than the screen.

## Step 4 — the API map

| PVSnesLib | OpenSNES | Note |
|---|---|---|
| `consoleInit()` | `consoleInit()` | Same |
| `consoleDrawText(x, y, s)` | `textPrintAt(x, y, s)` then `textFlush()` | The flush is what uploads the tilemap |
| `consoleSetTextVramBGAdr` etc. | `textInit`, `textLoadFont`, `textModeInit` | Split into explicit steps |
| `oamSet(...)` | `oamSet(id, x, y, tile, palette, priority, flags)` | Seven arguments; check the order |
| `oamSetEx`, `oamSetVisible` | `oamSetSize`, `oamHide`, `oamSetXY` | Split by concern |
| `oamInitGfxSet(...)` | `oamInitGfxSet(...)` | Same shape |
| `bgSetScroll(bg, x, y)` | `bgSetScroll(bg, x, y)` | The -1 is applied for you (trap 4) |
| `spcBoot`, `spcLoad`, `spcPlay` | `snesmodInit`, `snesmodLoadModule`, `snesmodPlay` | Module `snesmod`; the driver is the same SNESMOD |
| `spcProcess()` | `snesmodProcess()` | Call once per frame |
| `padsCurrent(pad)` | `padHeld(pad)` | Also `padPressed`, `padReleased`, `padRaw` |
| `pvsneslibfont` | `textLoadFont()` with your own font | No implicit font |

When a name is not in this table, search @ref api_index — it is organised by
what you are trying to do rather than by module.

## Step 5 — assets

The converters take the same shapes of input and, for graphics, the same flags
PVSnesLib users know:

```bash
gfx4snes -i sprite.png -p -t -s 16     # 16x16 sprites
gfx4snes -i bg.png -p -t -m            # background with tilemap
gfx4snes -i bg.png -p -t -m -R         # ... without tile reduction
```

Maps exported from Tiled go through `tmx2snes`, Aseprite sheets through
`aseprite2snes`, audio through `smconv` and `wav2brr`. Each has a page under
@ref tools, and `make/common.mk` runs them for you when the source file is
listed in the example's Makefile.

## Step 6 — when it still looks wrong

Port one screen, then compare it with the original before going further. The
project's own debugging rule applies to ports especially: **the bug is almost
never in the example**. Work down the layers — build system, library call,
asset conversion, compiler — instead of patching the symptom in game code.

- @ref tutorial_debugging — run both ROMs headless, dump VRAM, OAM and CGRAM,
  and compare them
- `KNOWN_LIMITATIONS.md` — every silent-failure trap in one list, with the
  mitigation for each
- @ref tutorial_far_ram — when a buffer needs to leave the 8 KB band

## Questions after a port

Common questions after a port are answered in @ref faq.
