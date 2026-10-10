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

Six checks, in this order; the build fails on the first four kinds of
failure, the others are information:

| Check | What it refuses | Why it is silent on hardware |
|---|---|---|
| **bank $00 ROM** | nothing by default: it reports the free bytes of the code bank and how much code the linker placed in the next banks (code runs from any bank). `--bank0-fail N` fails under N free bytes, for a project that wants it; asset payload that sits in bank $00 is named | — (until 2026-10-10 this was a failure under 1024 bytes, from the time C const data had to live there) |
| **C RAM band** | a RAM section that crosses or sits past `$2000`; fewer than `--ram-fail` bytes free; warns under `--ram-warn` with the three largest sections | plain C RAM addressing is bank-$00-implicit; above `$1FFF` it is wrong-banked or hits the registers (`FAR` is the way above) |
| **data-init sentinel** | `DataInitEnd` not at the end of `.data_init` | an object linked after `data_init_end.o` has globals that boot uninitialised |
| **bank-blind reads** | a symbol read with 16-bit addressing (`lda.w sym`, `lda.w #sym` without `#:sym`) that the linker placed in bank $01+ | the read returns garbage; pass the data as a far pointer or keep it in bank $00 |
| **NMI / WRAM port** | a write to `$2180-$2183` in a function reachable from `NmiHandler` or from a callback given to `nmiSet` | a mid-sequence interrupt corrupts the port's address and the main thread writes to the wrong place |
| **assets** | nothing: one line with the VRAM and CGRAM weight of the converted graphics on disk | an inventory, not a budget; `opensnes budget` measures the running scene |

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
against VRAM and CGRAM (an upper bound: a game seldom loads every asset at
once). It never fails on a figure — the thresholds are `check`'s — and
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
