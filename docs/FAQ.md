# Frequently asked questions {#faq}

Short answers, each pointing at the page that has the long one. If your ROM
misbehaves in a way that is not here, @ref troubleshooting walks the symptoms
and `KNOWN_LIMITATIONS.md` lists every trap that fails silently.

## Getting started

**Which file do I start from?**
@ref getting_started builds one ROM end to end. After that, @ref learning_path
is a route through the examples ordered by the question you are asking, not by
feature.

**Do I need to install a toolchain?**
No. `make` builds the compiler, the assembler, the asset tools and the library
from the repository. A release archive ships the same binaries prebuilt.

**Which emulator should I use?**
luna, pinned by the repository and installed by `scripts/install-luna.sh`. It
is the project's only test backend, it runs the coprocessors natively, and it
drives the automated suite. @ref tutorial_debugging covers stepping, memory
inspection and scripted input.

## The language

**Why is `int` two bytes?**
Because the 65816 is a 16-bit machine and a 16-bit `int` is what its
instructions do in one step. `long` is four bytes. Use the fixed-width names
from `<snes/types.h>` (`u8`, `u16`, `s16`, `u32`) and the question disappears.

**Can I pass a struct to a function?**
Not by value — the compiler refuses it with a message naming the feature, and
so it does for struct returns, struct assignment, variadic functions and inline
assembly. Pass a pointer instead. The refusals are deliberate: each one would
otherwise be a silent miscompilation.

**Can I use floating point?**
There is no FPU and no soft-float library. Use the fixed-point types: `fixed`
(8.8) from `<snes/math.h>`, `fixed32` (16.16) from `<snes/fixed32.h>`.
@ref tutorial_math has the arithmetic and the pitfalls.

**Is there a `printf`?**
Not in the core library, by design. Print to the screen with `textPrint`,
`textPrintU16` and `textPrintHex`, or to the host with the debug channel while
developing.

**Does `volatile` work?**
Yes, since chantier A2: a volatile load or store survives the optimiser and is
not coalesced. The library still uses plain globals for its NMI handshakes, for
cycle-cost reasons, but your MMIO patterns can rely on `volatile`.

## Things that go wrong silently

**My screen is black.**
Almost always the initialisation order: `setScreenOn()` must come last, after
every VRAM upload. See the checklist in @ref migrating_pvsneslib, trap 2.

**My tiles are garbage on the first frames.**
VRAM writes during active display are dropped by the PPU with no error. Upload
during forced blank or inside VBlank, and keep the per-frame DMA under ~4 KB —
@ref craft_frame_budget explains the ceiling and the two ways around it.

**My sprites have the wrong colours.**
Sprite palettes start at CGRAM offset 128, not 0.

**A `const` array reads garbage.**
You cast the `const` away and dereferenced it. Const data lives in the asset
banks and every C read of it is a far read; a plain pointer reads bank $00
instead. Keep the `const`, or hand the data to a library function that takes a
far pointer. The linker check fails the build on this pattern.

**A big buffer behaves strangely.**
Plain C globals must fit under `$2000`. Declare bulk buffers `FAR` so they land
in `$7E:2000-$FFFF` with bank-honouring code — @ref tutorial_far_ram.

**Everything scrolls one line off after a port.**
The library already subtracts the 1 that the PPU's missing scanline 0 requires.
Pass the row you want; keep your own -1 only in data that reaches the register
without the library, such as an HDMA table.

**My sound is silent.**
The audio driver has to be uploaded and serviced: initialise it, then call the
module's process function once per frame. @ref tutorial_audio walks the whole
path, and the audio examples are the shortest working reference.

## Building and shipping

**How large can my ROM be?**
The shipped memory maps cover LoROM, HiROM and the coprocessor layouts. Bank
$00 is code only and is guarded by a hard-fail free-space ratchet; your assets
belong in the asset banks, which the converters and the `ASSET_SECTION` macro
handle for you.

**How do I add a module to my build?**
Add its name to `LIB_MODULES` in the example Makefile. Dependencies are pulled
in automatically, and `make test-link-modules` proves every module links on its
own.

**Does my game run on a PAL console?**
It should, and the weekly PAL pass boots the whole example corpus at 312 lines
and 50 Hz to keep it that way. Read the region at runtime with `getRegion()` or
`isPAL()` when timing matters.

**Can I run this on real hardware?**
Yes. The ROMs are plain `.sfc` files; a flash cart runs them. The library's
hardware claims are arbitrated against a corpus of hardware documentation
before they are written down, and the emulator is cycle-accurate, but a
hardware check before release is still the honest final step.

## Contributing to OpenSNES

**Where do I report a bug or propose a change?**
`CONTRIBUTING.md` has the branch model, the commit format and the PR rules. The
short version: work on `develop`, keep one topic per pull request, and run
`make tests` plus `make lint` before you push.

**What do I run before pushing?**
`make clean && make`, then `make tests`, then `make lint`. Those are the same
gates CI runs, verbatim — that is deliberate, so a green local run means a
green CI run.
