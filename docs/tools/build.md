# Your project's Makefile {#tools_build}

Every example and every project scaffolded by `opensnes init` has a short
Makefile that sets a few variables and includes `make/common.mk`. This page
lists every variable `common.mk` reads, its default, and what it changes.
`devtools/check_doc_drift.py` fails when `common.mk` gains a `?=` variable
this page does not name.

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := mygame.sfc
ROM_NAME := MY GAME
USE_LIB  := 1
LIB_MODULES := console sprite dma input
include $(OPENSNES)/make/common.mk
```

Every variable can also be set on the command line for one build:
`make USE_FASTROM=1`.

## Sources

| Variable | Default | What it does |
|---|---|---|
| `CSRC` | `main.c` | C sources, compiled by `cc65816` and linked in order |
| `ASMSRC` | (none) | Extra 65816 assembly sources (data sections, hand-written routines) |
| `GFXSRC` | (none) | PNG files converted by `gfx4snes` into `.pic` / `.pal` (+ a C header) at build time |
| `SPRITE_SIZE` | `8` | Tile size passed to `gfx4snes -s` for the `GFXSRC` files |
| `SPCSRC` | (none) | SPC700 assembly (`*.spc700.asm`), assembled with `wla-spc700` into a `.spc700.bin` your 65816 code `.incbin`s |
| `GSUSRC` | (none) | Super FX assembly (`*.sfx`), assembled with `wla-superfx` into a `.sfx.bin` (needs `USE_SUPERFX := 1`) |

A `.wav` that an assembly file `.incbin`s as its `.brr` is converted by
`wav2brr` automatically (one-shot samples; run `wav2brr --loop` by hand for
looping ones).

## Library

| Variable | Default | What it does |
|---|---|---|
| `USE_LIB` | `0` | `1` links the OpenSNES library |
| `LIB_MODULES` | `console` | The library modules to link. Each module's own dependencies are added for you; a name that matches no module is an error that lists the available ones. `USE_SRAM`, `USE_SNESMOD`, `USE_SUPERFX` and `USE_DSP1` add their module |

## Cartridge

| Variable | Default | What it does |
|---|---|---|
| `USE_HIROM` | `0` | `1` builds a HiROM cartridge (64 KB banks instead of 32 KB) |
| `USE_FASTROM` | `0` | `1` marks the ROM as FastROM and runs the code from the fast mirror (~33 % more ROM bandwidth) |
| `ROM_BANKS` | `8` | Number of linker banks: 256 KB LoROM, 512 KB HiROM by default. The header's ROM size byte and the asset bank range follow it |
| `ROMSIZE` | from `ROM_BANKS` | The header's ROM size byte (`$FFD7`, 1 KB << n, rounded up to a power of two). Leave it computed |
| `ASSET_BANKS_RANGE` | `ROM_BANKS - 1` down to 1 | Banks the linker may use for `ASSET_SECTION` data and C const data, highest first. Leave it computed |
| `USE_SRAM` | `0` | `1` declares battery-backed save RAM and links the `sram` module |
| `SRAM_SIZE` | `3` | Save RAM size as the header byte `$FFD8`: 1 KB << n, n = 1..7 (3 = 8 KB) |

## Coprocessors

A cartridge carries one coprocessor: setting two of these is refused.

| Variable | Default | What it does |
|---|---|---|
| `USE_SA1` | `0` | `1` builds an SA-1 cartridge (its own memory map; not with `USE_HIROM`) |
| `USE_SUPERFX` | `0` | `1` builds a Super FX cartridge (LoROM-mapped; not with `USE_HIROM` or `USE_SRAM`) |
| `GSU_RAM_KB` | `64` | Super FX Game Pak RAM declared in the extended header (`$FFBD`) |
| `USE_DSP1` | `0` | `1` declares a DSP-1 cartridge and links the `dsp1` module (LoROM board; not with `USE_HIROM`) |

## Audio

| Variable | Default | What it does |
|---|---|---|
| `USE_SNESMOD` | `0` | `1` links the SNESMOD driver and converts `SOUNDBANK_SRC` with `smconv` |
| `SOUNDBANK_SRC` | (none) | Impulse Tracker `.it` files that make up the soundbank |
| `SOUNDBANK_OUT` | `soundbank` | Base name of the generated soundbank files |
| `SOUNDBANK_BANK` | `1` | ROM bank passed to `smconv -b` for the soundbank data |

## Build checks

Every link runs checks that catch silent failures (a const table read
through the wrong bank, RAM past `$2000`, the stack reaching the C
variables). Their thresholds are variables; lowering one is a decision to
write down, raising it is almost always wrong (`.claude/rules/bank0_budget.md`).

| Variable | Default | What it does |
|---|---|---|
| `BANK0_FAIL_THRESHOLD` | `1024` | Fail the link when bank $00 (code) has fewer free bytes than this |
| `RAM_FAIL_THRESHOLD` | `512` | Fail the link when the plain C RAM band (`$0000-$1FFF`) has fewer free bytes than this |
| `RAM_WARN_THRESHOLD` | `1024` | Warn below this many free bytes in that band |

Each check has a bypass for debugging, never for a commit: `SKIP_LINT=1`
(the clang syntax pass over your C), `SKIP_BANK0_CHECK=1`,
`SKIP_RAM_CHECK=1`, `SKIP_BANKREAD_CHECK=1` (bank-blind reads of const
data), `SKIP_NMI_RACE_CHECK=1` (WRAM data port use in NMI code),
`SKIP_ASSET_BUDGET=1`.
