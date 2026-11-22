# opensnes-rom — is the ROM good? {#tools_opensnes_rom}

The tool that speaks last in a build (@ref tools_conventions). It runs
the **post-link checks** every OpenSNES project gets after `wlalink`,
reads a ROM's **cartridge header and checksum**, and prints the game's
**budget**. It is what lets a game developer build without Python: the
five scripts the checks lived in stayed with the contributors.

## Check

```sh
opensnes-rom check game.sfc                                  # the .sym and the .c.asm beside it
opensnes-rom check game.sfc --bank0-fail 0 --ram-fail 512    # what make/common.mk passes
opensnes-rom check game.sfc --json
```

Seven checks, in this order; every one but the asset line can fail the
build. A ROM the build's check refuses is deleted, so that a second `make`
does not take it for up to date: run `opensnes-rom check` on its `.sym`,
which stays.

| Check | What it refuses | Why it is silent on hardware |
|---|---|---|
| **bank $00 ROM** | nothing by default: it reports the free bytes of the code bank and how much code the linker placed in the next banks (code runs from any bank). `--bank0-fail N` fails under N free bytes, for a project that wants it; asset payload that sits in bank $00 is named | — |
| **C RAM band** | a RAM section that crosses or sits past `$2000`; fewer than `--ram-fail` bytes free; warns under `--ram-warn` with the three largest sections | plain C RAM addressing is bank-$00-implicit; above `$1FFF` it is wrong-banked or hits the registers (`FAR` is the way above) |
| **data-init sentinel** | `DataInitEnd` not at the end of `.data_init` | an object linked after `data_init_end.o` has globals that boot uninitialised |
| **SA-1 boot** | `SA1Start` outside the ROM of bank $00 (an SA-1 ROM only) | the SA-1 reset vector is 16-bit: the SA-1 never runs a line, the S-CPU waits for it, and a SUPERFREE boot section gets there as soon as bank $00 fills |
| **bank-blind reads** | a symbol read with 16-bit addressing (`lda.w sym`, `lda.w #sym` without `#:sym`) that the linker placed in bank $01+ | the read returns garbage; pass the data as a far pointer or keep it in bank $00 |
| **NMI / WRAM port** | a write to `$2180-$2183` in a function reachable from `NmiHandler` or from a callback given to `nmiSet` or `nmiSetLag` | a mid-sequence interrupt corrupts the port's address and the main thread writes to the wrong place |
| **assets** | nothing: one line with the VRAM and CGRAM weight of the converted graphics on disk; with one folder per level under `res/`, the heaviest level with what `res/` shares | an inventory, not a budget; an asset whose settings say `vram = N` counts N (a sheet streamed through a cache: `vram = 0`); `opensnes budget` measures the running scene |

The figures print at every link (`OK: bank $00 ROM (code): 19177 bytes
free`, `OK: C RAM band $0000-$1FFF: 4832 bytes free …`): they are the
instrument that turns "will we hit the ceiling?" into a number.

## Output and exit codes

0 every check passed; 1 one failed (the message names the symbol, the
section or the function, and what to do); 3 no `.sym` beside the ROM.
`--json` gives one object: the free bytes of each band, each verdict, the
asset weight. `--no-bank-reads`, `--no-nmi-race`, `--no-assets` skip a
check; `make/common.mk` maps `SKIP_BANKREAD_CHECK=1` and the others onto
them.

Once it has read the `.sym`, the check removes wlalink's RAM-usage markers
from it (the `RAM_USAGE_SLOT_*_START` / `_END` rows of `[labels]`). The
linker writes them as bank-$00 labels whatever their bank (`$7E:2000` comes
out as `00:2000`), and a profiler that folds PCs onto the nearest label
credits code to them: `luna profile` named `RAM_USAGE_SLOT_2_BANK_126_START`
as a routine. The checks take the RAM figures from `[ramsections]`, so
nothing reads the markers; `--keep-ram-markers` leaves the file as the
linker wrote it.

## Static frames: place, and check --objects

```sh
opensnes-rom place linkfile                       # writes sframe.asm beside the linkfile
opensnes-rom check game.sfc --objects linkfile    # the seven checks, and the frames
```

A static frame keeps a C function's values at a fixed place of the direct
page, `sf.<function>.f`, instead of on the stack. Two functions may share
those bytes only if they can never be active together, and a function with
a static frame must never be active twice; the compiler sees one file, so
both are decided here, over the whole program, from the objects the link
takes (the `linkfile` that `make` writes). The build does not run these
yet, and the compiler gives no function a static frame: `place` writes a
window with no section and no label.

Every object has its text beside it: `X.c.wrap.asm` or `X.wrap.asm` in the
project, `X.c.asm` and `X-asm.text.asm` in `lib/build/<flavour>/`,
`crt0.text.asm`. The compiler writes a call table into every C text
(comment lines `; sframe fn`, `; sframe call`, `; sframe addr`). A routine
of assembly is closed when the routines it calls are closed and it does
not jump through a pointer or return to an address it pushed; anything
else, an object without a text, or a call to a symbol no text defines, may
reach any exported or address-taken function.

`place` reads the C tables and the assembly, refuses a function with a
static frame that can call back into itself (it names the path) and a
window too small, places each frame just past the deepest frame that can
be active under it, and writes `sframe.asm`. On a refusal it writes
nothing and removes an old `sframe.asm`, so the link stops on the missing
labels whatever the Makefile does with the exit code. `--what-if` gives
every function the compiler could frame its frame, reports the refusals
and the deepest chain, and writes nothing.

`check --objects` reads the same objects another way: the calls, the
addresses taken and the bytes of each frame come from the instruction
lines, not from the tables. It refuses a table that differs from its text,
an `sf.*` label of the `.sym` no table claims (or the reverse), a frame
used past its size or without `.b`, a function with a static frame that
can be active twice, two frames that overlap while one can run under the
other, a frame past `$FF`, outside its window or on another block of the
direct page (`.registers`, `tcc__lf`, `.snesmod_zp`), and a frame an NMI
callback can reach that ends past the NMI page.

## Inspect

```sh
opensnes-rom inspect game.sfc
# game.sfc: "MY GAME" — LoROM, 256 KB, 8 KB battery RAM, NTSC (country $01), version 1.0
#   checksum $82CA ok · crc32 25fea782 · sha1 0beaf360885c696872e052ec80fc341d5ba93800
```

What the cartridge header says, read back from the ROM as built: the
title, the mapping (LoROM, HiROM, ExHiROM, FastROM), the coprocessor, the
image size against the size the header declares, the save RAM and whether
a battery keeps it, the country code and the region it implies, the
version. Then the **checksum**: the 16-bit sum of the image, recomputed
and compared with the one in the header and its complement; an image that
is not a power of two is summed as its largest power-of-two part plus the
rest repeated. The header layout and the checksum rule are those of the
[SNESdev wiki](https://snes.nesdev.org/wiki/ROM_header) and of fullsnes.
The CRC32 and the SHA-1 are what a tester, a flash-cart database or a
publisher identifies a build by.

Exit 1 when the file has no header or its checksum is wrong: `wlalink`
writes the checksum at link time, so a wrong one means something edited
the ROM afterwards. A 512-byte copier header in front of the image is
skipped and said. `--json` gives every field for each ROM.

## Budget

```sh
opensnes-rom budget game.sfc          # reads game.sym beside it
```

```
game.sfc — "RPG TEMPLATE", LoROM, 256 KB in 8 banks of 32 KB
ROM
  bank $00 (code)              22604 / 32768   bytes    69%   10164 free
  bank $07                     24781 / 32768   bytes    76%   7987 free
  6 empty banks
  whole ROM                    47385 / 262144  bytes    18%   214759 free
RAM
  C variables $0000-$1FFF       5408 / 8192    bytes    66%   2784 free
  FAR $7E:2000-$FFFF               0 / 57344   bytes     0%   57344 free
Video (the assets of the project, if all were loaded at once)
  VRAM                         20768 / 65536   bytes    32%   44768 free
  CGRAM                          128 / 256     colours  50%   128 free
```

One report where the build prints several lines: what the linker placed
in each ROM bank, the two RAM bands C can use (@ref tutorial_far_ram), the
cartridge's save RAM, and the weight of the project's converted assets
against VRAM and CGRAM: every asset together, an upper bound, or, when
`res/` has one folder per level, the heaviest level with what `res/` and the
project's folder share. An asset whose settings file says `vram = N` (before
its table) counts N bytes instead of its files' size: the bytes a cache
reserves for a sprite sheet streamed through it, 0 for a sheet never
resident. It never fails on a figure — the thresholds are `check`'s — and
`--json` gives the same numbers per bank. `opensnes budget` builds the
project and runs it (@ref tools_opensnes).

Not in the report yet, because a link cannot know them: the time the NMI
handler takes in VBlank and the size of the SPC700 sound data; luna
measures the first (`luna profile`, @ref tools_luna).

## Where the checks came from

`symmap.py --check-bank0-overflow / --check-ram-budget / --check-data-init`,
`check_bank_reads.py`, `check_nmi_wram_race.py` and
`asset_budget.py --oneline` in `devtools/`. The port was compared against
them on every built ROM of the repository (99: same verdicts, same free
byte counts, same far-band figures). The Python stays for the contributor
gates that use it (`symmap.py --check-overlap` on the release ROMs, the
unit tests).
