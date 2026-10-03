# Changelog

All notable changes to OpenSNES are documented in this file.

## [Unreleased]

### Added
- test(luna-test): **`make hardware-preflight`** replays the 23 ROMs of the
  real-console protocol (`docs/HARDWARE_VERIFICATION.md`) on luna from
  pseudo-random RAM (three seeds) and under PAL, and checks every VRAM DMA
  byte lands in blank — the two cheapest ways a ROM green on luna's
  defaults fails on a console. It reads the protocol's table like
  `make hardware-kit`, so the kit and the preflight cannot drift apart.
  First run: 23 of 23 rows ready (`ROWS=1-7` for the gate rows).
- feat(lib,build): **a Super FX game can save.** `USE_SRAM := 1` with
  `USE_SUPERFX := 1` was refused by the build; it now declares a battery
  (cartridge type `$15`) and the `sram` module reads and writes the GSU's
  Game Pak RAM, which is what such a cartridge keeps (there is no separate
  save chip). The capacity comes from `$FFBD` (`GSU_RAM_KB`), offsets count
  from `$70:0000`, and the game chooses a region its framebuffers do not
  use (`sramSaveOffset` / `sramLoadOffset`). The RAM is shared with the
  GSU: the module clears RAN for the transfer and puts SCMR back through
  `gsu_scmr_live`, so a save in the middle of a job works. Tested on the GSU
  fixture — save and read back idle, then during a cached job that still
  ends with its results; without the hand-over the read-back is wrong and
  luna counts 8 GSU bus violations — and across a power cycle
  (`f_gsu_save_write.toml`, `g_gsu_save_read.toml`). This answers luna's
  question of 2026-10-02 and closes the Super FX runtime chantier.
- feat(build): **`GSU_BANK` links the Super FX program at its real ROM
  address.** Until now a `.sfx` was assembled at 0 and placed by the linker
  where it fitted, so only position-independent GSU code was right: an
  absolute jump or a table of the program read through `ROMB` / `GETB`
  pointed elsewhere. `GSU_BANK := n` in the Makefile assembles the program
  at `$8000` and `GSU_SECTION` (new macro, `templates/assets.inc`; an
  `ASSET_SECTION` when `GSU_BANK` is unset) forces it to `$n:8000`, where
  the GSU reads it. The generated `.sfx.h` keeps offsets, so `gsuCall()`
  and `gsuStartCached()` do not change. One `.sfx` per ROM in that mode.
  The GSU fixture is built this way and gains `rom_job` (table read and
  absolute jump): `$3CA5`; the same binary placed elsewhere returns 0.
  This closes the last part of the Super FX runtime's phase F.
- feat(lib): **`oamDrawMetasprite(id, x, y, frame, &style, flip)`** replaces
  `oamDrawMeta()` (7 arguments) and `oamDrawMetaFlip()` (11) (API decision
  on principle 4, shape chosen by the owner). A `MetaspriteStyle`, usually
  `static const`, holds what does not change between frames — base tile,
  palette, OBJ size, and for a mirrored draw the piece size and the box —
  while the frame (what `animTickMeta()` returns), the position and
  `OBJ_FLIPX` / `OBJ_FLIPY` are given at each call. The two old names are
  deprecated until 1.0. **It also fixes the mirrored draw of 32-pixel
  pieces:** `oamDrawMetaFlip()` assumes a piece is 16 pixels when large and
  8 when small whatever the OBJSEL mode; the style carries `pieceSize`
  (fixture: a 32-pixel piece in a 64-wide box lands at x + 32, the old
  form puts it at x + 48). Cost measured and written in `docs/PERF.md`:
  about 2,000 master cycles per call, 0.6 % of a frame. `metasprite` and
  `aseprite_pipeline` are migrated with identical images. Twelve WRAM
  streams are re-captured: `sprite.c` grew by `$341` bytes and what moved
  in RAM is ROM addresses (checked byte by byte on `basics/random` — the
  compiler's scratch register `tcc__r9` at one frame — and on `likemario` —
  the dynamic engine's `dynamic_flush_hook` pointer).
- feat(lib): **`dsp1SetCamera(const Dsp1Camera *)`** replaces
  `dsp1Parameter()` and its seven positional arguments (API decision on
  principle 4, more than five arguments). `Dsp1Camera` holds the command's
  seven inputs under the manual's names (`x`, `y`, `z`, `lfe`, `les`,
  `aas`, `azs`): a fixed view is one `static const`, a moving camera is one
  struct whose position and heading the game changes. The struct is the
  command's 14 bytes in order, so the assembly sends it as it lies.
  `dsp1Parameter()` is deprecated and stays until 1.0; the DSP-1 fixture
  checks the two give the same four output words (18/18). `dsp1_cube` and
  `dsp1_ground` are migrated with identical images; their two WRAM streams
  are re-captured (the camera variables of `dsp1_ground` are now one struct,
  and the call parks a pointer in the direct page).
- feat(tools): **smconv reads compressed Impulse Tracker samples.** IT 2.14
  and IT 2.15 compression, 8-bit and 16-bit, are decoded (bit 2 of the
  sample's `Cvt` selects IT 2.15, as OpenMPT and Schism Tracker read it), so
  a module saved with "compress samples" no longer has to be re-saved. The
  decoder is a port of modlib's (Mukunda Johnson, MIT — `ATTRIBUTION.md`).
  The golden test converts a real IT 2.14 file (`reflection.it`, whose
  decoded PCM matches modlib's reference byte for byte) and compressed
  twins of `pollen8.it` made by a test-only encoder, which must give the
  committed soundbank; IT 2.15, 16-bit and multi-block samples are covered
  by those round trips only.
- feat(build): **`ROM_REGION`** (`ntsc` default, `pal`, `jp`) — the header's
  country byte (`$FFD9`: `$01`, `$02`, `$00`; fullsnes, snesdev-wiki). It
  was `$01` on every ROM, so a European game had no way to declare itself
  PAL. Default builds are byte-identical (89/89 ROMs compared).
- test(luna-test): **a PAL pass on the games** — `make test-pal` replays the
  six scripted manifests of tetris, breakout, likemario, shmup_1942 and rpg
  at 50 Hz on the NTSC-built ROMs, with `region = "pal"` in the
  manifest (an import cartridge on a PAL console) and `stat78 = $13`
  asserted so a 60 Hz run fails: 6/6. The manifests are derived from the
  NTSC ones at run time. `ROM_REGION=pal` itself is checked on one game:
  tetris built as a PAL cartridge is PAL for luna with nothing forced and
  passes its manifest. Last item of action 39.
- feat(examples,docs): **`backgrounds/mode6` test card (B)** for a question no
  reference answers: does bit 3 of a hi-res horizontal offset (8 half-pixels,
  inside a 16-wide tile) move the column? luna and ares say half a tile,
  Mesen2 drops it (luna's report of 2026-10-02). The card puts 8 on every odd
  column and nothing else; `backgrounds_mode6_card.toml` pins the table, the
  README explains the divergence, and the hardware protocol gains row 23
  (a photo of the card on a console).
- feat(examples): **`backgrounds/mode6`** — a hi-res 4bpp layer with
  offset-per-tile: Mode 6 reads the table like Mode 2 (an H row, a V row),
  with 16-half-pixel columns; A moves the wave from the vertical row to the
  horizontal one. One-half-pixel stripes built at run time. Its manifest pins
  both rows in VRAM, both screens and BG3VOFS. Last mode gap of action 39.
- feat(examples): **`backgrounds/mode4`** — a 256-colour layer with
  offset-per-tile: Mode 4's single row of words, each vertical (bit 15) or
  horizontal; A switches the wave from one to the other. 8bpp tiles built
  at run time. Its manifest pins the words in VRAM and BG3VOFS.
- test(luna-test): **an animated example must animate** — `luna_runner.py`
  refuses a capture, and fails a baseline, whose capture points
  (`frames = [a, b]`) are all the same frame; captures are staged, so a
  refused one no longer overwrites the baseline PNGs.
- feat(lib): **`mode7SetExtBg(on)`** — Mode 7 EXTBG (SETINI bit 6): BG2
  shows the same plane with bit 7 of each pixel as its priority, a second
  layer around the sprites. Composed through the SETINI shadow the
  `videoSet*` setters share.
- feat(examples): **`mode7/extbg`** — a sprite rolls over a floor and
  behind pillars drawn in one Mode 7 plane, split by bit 7; A toggles
  EXTBG. Plane built at run time in a `FAR` buffer. Manifest pins SETINI,
  TM, the 1:1 matrix and the scroll.
- feat(examples): **`color/pseudo_hires`** — a 50 % blend of two layers
  without colour math: SETINI bit 3 puts the sub screen on the even columns
  of a 512-pixel line and the main screen on the odd ones (snesdev-wiki,
  anomie; checked in luna's native output). Press A to toggle; tiles built
  at run time, no asset. Its manifest pins SETINI, the screen designations
  and the scroll.

### Deprecated

- **The 47 names below build and warn in this release and are removed at
  1.0** (`hdmaEnable` / `hdmaDisable` come back at 1.0 taking a channel
  number). The clang pre-pass reports each use; a build without clang
  reports nothing, so compare against this list before upgrading. Every
  replacement exists in this release.

  | Deprecated | Header | Use instead |
  |---|---|---|
  | `audioUpdate` | `audio.h` | it does nothing |
  | `colorMathEnable` | `colormath.h` | `colorMathSetLayers` |
  | `COLORMATH_BG1` | `colormath.h` | `LAYER_BG1` |
  | `COLORMATH_BG2` | `colormath.h` | `LAYER_BG2` |
  | `COLORMATH_BG3` | `colormath.h` | `LAYER_BG3` |
  | `COLORMATH_BG4` | `colormath.h` | `LAYER_BG4` |
  | `COLORMATH_OBJ` | `colormath.h` | `LAYER_OBJ` |
  | `consoleInitEx` | `console.h` | `consoleInit` |
  | `getRegion` | `console.h` | `isPAL` |
  | `rand` | `console.h` | `rngNext` |
  | `srand` | `console.h` | `rngSeed` |
  | `dmaCopyVramBank` | `dma.h` | dmaCopyVram() takes the bank from the source pointer |
  | `dmaCopyCGramBank` | `dma.h` | dmaCopyCGram() takes the bank from the source pointer |
  | `dsp1Parameter` | `dsp1.h` | `dsp1SetCamera` |
  | `dsp1Present` | `dsp1.h` | `dsp1IsPresent` |
  | `hdmaSetupBank` | `hdma.h` | hdmaSetup() takes the bank from the table pointer |
  | `hdmaEnable` | `hdma.h` | `hdmaEnableMask` |
  | `hdmaDisable` | `hdma.h` | `hdmaDisableMask` |
  | `padRaw` | `input.h` | `padHeld` |
  | `nmiSetBank` | `interrupt.h` | nmiSet() takes the bank from the function pointer |
  | `irqSetBank` | `interrupt.h` | irqSet() takes the bank from the handler pointer |
  | `LzssDecodeVram` | `lzss.h` | `lzssDecodeVram` |
  | `ease_in_quad` | `math.h` | `easeInQuad` |
  | `ease_out_quad` | `math.h` | `easeOutQuad` |
  | `mode7SetPivot` | `mode7.h` | `mode7SetCenter` |
  | `mosaicEnable` | `mosaic.h` | `mosaicSetLayers` |
  | `MOSAIC_BG1` | `mosaic.h` | `LAYER_BG1` |
  | `MOSAIC_BG2` | `mosaic.h` | `LAYER_BG2` |
  | `MOSAIC_BG3` | `mosaic.h` | `LAYER_BG3` |
  | `MOSAIC_BG4` | `mosaic.h` | `LAYER_BG4` |
  | `profileGetFrameCount` | `profile.h` | `getFrameCount` |
  | `BGMODE_MODE0` | `registers.h` | `BG_MODE0` |
  | `BGMODE_MODE1` | `registers.h` | `BG_MODE1` |
  | `BGMODE_MODE2` | `registers.h` | `BG_MODE2` |
  | `BGMODE_MODE3` | `registers.h` | `BG_MODE3` |
  | `BGMODE_MODE7` | `registers.h` | `BG_MODE7` |
  | `sa1Init` | `sa1.h` | `sa1IsReady` |
  | `snesmodSetSoundTable` | `snesmod.h` | no SDK call starts a stream |
  | `snesmodAllocateSoundRegion` | `snesmod.h` | no SDK call starts a stream |
  | `oamDrawMeta` | `sprite.h` | `oamDrawMetasprite` |
  | `oamDrawMetaFlip` | `sprite.h` | `oamDrawMetasprite` |
  | `NAME` | `types.h` | use ... |
  | `WINDOW_BG1` | `window.h` | `LAYER_BG1` |
  | `WINDOW_BG2` | `window.h` | `LAYER_BG2` |
  | `WINDOW_BG3` | `window.h` | `LAYER_BG3` |
  | `WINDOW_BG4` | `window.h` | `LAYER_BG4` |
  | `WINDOW_OBJ` | `window.h` | `LAYER_OBJ` |

### Fixed
- fix(lib): **`apuUpload()` with a size of 0 sends nothing.** It sent one
  byte before testing the end, so 0 meant 65 536 bytes (library audit,
  row 18).
- fix(lib): **`audioSetVoiceVolume()` clamps to 0-127.** The DSP's voice
  volumes are signed: 128 and above inverted the phase (library audit,
  row 19).
- fix(lib): **`apuWaitBoot()` is bounded and `audioInit()` returns
  `AUDIO_ERR_TIMEOUT` as its header promised.** Called when the IPL is no
  longer running (a driver already is), the wait for `$AA` / `$BB` never
  ended and the CPU hung. It gives up after about seven frames and returns
  1; `audioInit()` returns the timeout. libtest vector `r_apu_boot_again`
  (library audit, row 17). Five audio hashes re-captured: the poll loop
  costs a few cycles per iteration before the IPL answers, so the upload
  starts a few samples later; `luna diff --audio` before/after: `MATCH`,
  0.00 to 0.19 % per window, first sample within 2.
- fix(lib): **`hdmaIrisWipe()`, `hdmaBrightnessGradient()` and
  `hdmaColorGradient()` called again while they run** only move the
  channel's table pointer, read at the next frame. They set the channel up
  again, which resets its address and line counter and restarts the table
  at the next HBlank: for the rest of that frame the bottom of the screen
  showed the top of the table (luna, `hdma_helpers` frame 150; library
  audit, row 2).
- fix(lib): **the dynamic sprite engine's VRAM queue is bounded.** It holds
  128 entries and the NMI drains seven a frame; the index advanced without
  a bound, so more than 128 pending refreshes overwrote the engine's own
  state, index included. A full queue now leaves the sprite's refresh flag
  set and tries again next frame (library audit, row 6).
- fix(tools): **`sa1_patch` keeps the header checksum right.** Patching
  the map mode byte to `$23` added 3 to the ROM's byte sum; every SA-1 ROM
  shipped with a checksum off by 3 (luna and most emulators check only
  that the two fields are complements, a console or a strict tool sees
  it). The pair is corrected by the difference (build audit, S6).
- fix(tools): **`smconv` fails on a module with more than 8 channels or
  too big for SPC RAM** (it printed "error" and exited 0 with a truncated
  soundbank); **`wav2brr` warns, with or without `-v`, about a source
  above 32 kHz**, and says what happens: the DSP plays it at 32 kHz, so it
  comes out lower and slower (the old `-v`-only note said
  "higher-pitched") (build audit, S9, S10).
- fix(compiler): **the preprocessor runs with `-undef -nostdinc`.** The
  host's predefined macros (`__x86_64__`, `__aarch64__`, `__linux__`) made
  the same source give a different ROM on each build machine, and the
  host's `<stdint.h>` was read with cproc's sizes: `int32_t` came out 2
  bytes, `int64_t` 4, without a word (build audit, S2). A `#include
  <stdint.h>` now fails at the preprocessor; the fixed-width types are
  `snes/types.h`'s.
- fix(build): **`ROM_BANKS` is bounded per mapping** (LoROM 8-126, HiROM,
  SA-1 and Super FX 8-64, DSP-1 8-32). Past those the linker placed data
  in memory the cartridge does not map as ROM — a string at `$7E:8000`
  (WRAM) on a 127-bank LoROM, `.rodata` at `$40:0000` (BW-RAM) on a 65-bank
  SA-1, a HiROM label past bank `$7F` — and the build stayed green, the
  text just vanished (build audit, S1). `GSU_RAM_KB` must be 32, 64 or 128
  (`48` declared 32 KB, a typo gave a Python traceback and `$FFBD = 0`).
- fix(build): **a changed `GSU_BANK` reassembles the GSU program**, and
  the data-init objects and the soundbank object rebuild when the project
  configuration changes: `make GSU_BANK=2` on a tree built with 1 moved
  the section but kept a program that set `ROMB` to 1, and `make
  USE_HIROM=1` after a LoROM build gave a 256 KB LoROM ROM (build audit,
  S3, S4).
- fix(lib): **the map was drawn one line too low since 2026-09-12.** The
  VOFS change of that day added a `y - 1` in `mapVblank`, but the map
  module's offset already carried one (PVSnesLib's `clc / sbc`, `dec a`):
  BG1 received `y - 2`. luna read BG1's vertical scroll at 1022 where BG2
  to BG4 read 1023. Four images re-captured (`map_scroll`, `tiled`,
  `slope_collision`, `mapandobjects`): each new image is the old one moved
  up one line (99.8 to 100 % of the pixels).
- fix(lib): **`oamHide()` and `oamClear()` park sprites at X = 257, not
  256.** The PPU counts an OBJ at X = 256 as X = 0 for its per-line range
  and time tests (anomie-regs, "Drawing the Sprites"): a hidden 32- or
  64-pixel sprite still took one of the 32 slots and its tiles on lines
  0-47, which could starve the real sprites there. The dynamic engine
  always hid at 257. Nothing changes on screen; every WRAM stream moves
  (the OAM shadow byte).
- fix(lib): **`gsuLaunch()` and `gsuStartCached()` drop CFGR's MS0 (fast
  multiply) bit**, which fullsnes says must be zero in 21 MHz mode — the
  mode both select. `gsu_cfgr = $A0` is still accepted; the bit is masked
  (the GSU fixture asks `$A0` and luna reads `$80` back).
- fix(lib): **`gsuSetupHdmaBlanking()` with a band of 0 lines.** A top band
  of 0 wrote a count of 0 as the table's first entry, which ends an HDMA
  table: the channel did nothing and `gsuDmaFullFrame()` transferred the
  whole frame on visible lines (luna `--dma-trace`: 1 844 726 of
  2 326 528 VRAM bytes outside blank with `(0, 80)`). A band of 0 has no
  entry now, a band of 128 or more takes two, counts are clamped at 224.
  The GSU fixture reads the table for `(0, 80)`.
- fix(lib): **`consoleInit()` latches the H/V counters before seeding the
  RNG.** Read unlatched, OPHCT and OPVCT were 0 and the seed was STAT78
  alone: the same `rngNext()` sequence at every boot. `basics/random`'s
  image re-captured (a different layout from a different seed).
- fix(lib): **`fixLerp()` over a difference of 128.0 or more.** `b - a` was
  taken on 16 bits with bit 15 as its sign: `fixLerp(FIX(-64), FIX(64),
  128)` gave -128.0 instead of 0. The difference is read over 17 bits
  (overflow flag). `fix32Lerp()` (inline, `fixed32.h`) has the same shape
  on 33 bits and is not changed here.
- fix(compiler): **four silent miscompilations in cproc, all on common
  idioms**, found by the pre-1.0 hunting campaign and reproduced on luna
  (`.claude/notes/reviews/2026-10-03_audit/A_compiler.md`). `++` / `--` on
  a `FAR` object read bank $7E and wrote bank $00 (`fcount = 10;
  fcount++; ++fcount;` gave 10, and a byte landed in `$00:2000-$7FFF`, the
  hardware registers); the same store dropped `volat` on a volatile. A
  whole-struct copy with a 4-aligned member copied two of its four bytes,
  and `= {0}` on a `u32` array or a struct with an `s32` left the upper
  halves unwritten: both used upstream's chunk table, where a QBE word is
  4 bytes (here it is 2). A bit-field of a `FAR` object, or of const data
  read through a pointer, was read in bank $00. None of the four occurs in
  the examples, the library or the fixtures (IR scan). **Struct assignment
  (`a = b;`) now works** for bank-$00 and ROM objects — it was refused by
  accident (halfword ops the backend has no lowering for) — and is refused
  on purpose for a `FAR` object on either side; **struct returns by value
  are refused on purpose** (they compiled to a copy of zeros once the
  accident was gone). New runtime fixture `devtools/compiler-tests/runtime/
  d_quals` (16 cells on luna); `struct_assign` moves from the refusal pins
  to a positive check. Every example matches its previous build frame for
  frame (`diff_corpus` 89/89).
- fix(lib): **`snesmodInit()` leaves NMITIMEN as it found it.** It ended on
  `$81`, so an H or V timer IRQ enabled before the audio driver was loaded
  stopped firing, silently. It now restores the lib's software copy of the
  register. `libtest_snesmod`: 0 IRQs in the 10 frames after the call
  before, 10 after.
- fix(lib): **`snesmodProcess()` no longer latches the H/V counters, waits
  its real five scanlines, and the SNESMOD command queue cannot wrap.** With
  several commands queued the wait loop read OPVCT once per turn (a
  read-twice register): the budget ran out in two or three lines, OPVCT's
  read pointer stayed shifted for the next reader, and the latch flag was
  left set, which `crt0` takes for a Super Scope shot. An 86th queued command
  wrapped the 256-byte queue and the driver stopped answering. The wait now
  counts H-blank edges (`$4212` bit 6), a command that does not fit is
  dropped, the stray `cli` in `QueueMessage` / `snesmodInit` is gone and
  `snesmodGetPosition()` reads until two reads agree; commands with no
  parameter queue 0 in their unused bytes instead of what a scratch variable
  held. Found by snes-rag
  reading the upstream driver; measured and pinned by `libtest_snesmod`
  (before: latch flag `$40`, 3 lines, depth 44 after 100 sends, never
  drained; after: 0, 6, 255, drained in 40 frames).
- fix(tools): **smconv crashed on a module with a compressed sample.** It
  printed "unsupported compressed samples", kept a NULL buffer with the
  declared length and segfaulted in the BRR encoder (exit 139). Compressed
  samples are decoded now, and a corrupt stream (block past the end of the
  file, a bit width the format does not have, a length the file cannot
  hold) ends in `sample '<name>': corrupt compressed data (block N)`, exit
  1 and no output file.
- fix(runtime,lib): **offset-per-tile was off since 2026-09-12** — the NMI
  wrote BG3's VOFS as `y - 1` like any displayed layer, but in Modes 2, 4
  and 6 BG3 is the offset table and VOFS selects its rows (snesdev-wiki,
  *Offset-per-tile*): `bgSetScroll(2, 0, 0)` read rows 31 and 0, all
  zeros, and `backgrounds/mode2`'s bands went flat. `setMode` now records
  whether the mode uses offset-per-tile (`bg3_opt`, a crt0 sysvar) and
  re-syncs BG3's scroll; the NMI writes BG3's VOFS raw in those modes. The
  flat picture had been re-captured as `mode2`'s baseline at both of its
  capture points; `diff_corpus`: `mode2` is the only example that changes.

### Changed
- test(luna-test): **luna pinned at v1.32.0** (`luna diff --audio`, the two
  clearer `region` messages). `docs/tools/luna.md` regenerated. The three
  audio-comparison runs of our request were replayed on the published
  binary: half-volume module `DIFF` at 75.93 %, same ROM `MATCH`, scripted
  `snesmod_sfx` `MATCH`. `.claude/rules/testing.md` now asks every commit
  that re-captures `baselines/audio.json` to quote `luna diff --audio`
  between the build before and the one after.
- refactor(lib,docs): **`mode7SetPivot()` is deprecated in favour of
  `mode7SetCenter()`** (owner decision, completing D5). It writes the same
  two registers from two `u8`; the tutorial said it took screen coordinates
  and computed centre and scroll, which the code never did. `mode7.md` now
  gives the rule instead: the centre shows on screen at (centre − scroll),
  so a rotation around the middle of the screen wants
  `mode7SetCenter(scrollX + 128, scrollY + 112)` (formula: anomie-regs,
  M7X; snesdev-wiki, Mode 7 transform, chunk `bf5cb2a48a63c1aa`).
- **BREAKING** refactor(lib): **twelve internal variables leave the public
  headers** (API decision D4, last part): the ten `lkup*` VRAM lookup
  tables of the dynamic sprite engine (`sprite.h`; only the engine's
  assembly reads them) and the `gsuPresent()` diagnostics `gsu_pres_frames`
  / `gsu_pres_last` (`superfx.h`). No example and no page used them; a
  project that did declares them `extern` itself. Five Super FX variables
  that the same list would have removed **stay public**: `gsu_stop_irqs`,
  `gsu_owns_cart`, `gsu_prog_bank`, `gsu_prog_addr` and `gsu_scmr_live` are
  the documented contract for code that starts a GSU job by itself
  (`docs/tutorials/superfx.md`).
- docs: `PHILOSOPHY.md`, principle 4 — a struct is asked of a new function
  with more than **five** arguments, not three (API decision).
- docs: **`WaitForVBlank()` keeps its name** (API decision D2). The one
  function of the library with a capital first letter stays as it is — it
  is the name every PVSnesLib port arrives with — and `PHILOSOPHY.md`, the
  migration guide and `console.h` now say it is the deliberate exception.
- refactor(lib,examples,docs): **`hdmaEnableMask()` / `hdmaDisableMask()`
  replace `hdmaEnable()` / `hdmaDisable()`** (API decision D1, first of two
  steps). The pair took a bit mask in a header where every other function
  takes a channel number, and the repository called it in fourteen
  spellings. The new names say what the argument is; the old ones are the
  same entry points, deprecated, and every call in the examples, the docs
  and the fixtures is migrated. **At 1.0 `hdmaEnable(channel)` comes back
  taking a channel number**: a call left as `hdmaEnable(0x40)` will then
  mean something else, which is what the warning is for.
- refactor(lib,docs): **the dead API is deprecated** (API decision on dead
  API). `audioUpdate()` (does nothing), `consoleInitEx()` (ignores its
  argument, is `consoleInit()`), `snesmodSetSoundTable()` and
  `snesmodAllocateSoundRegion()` (prepare a stream that no SDK call can
  start) build and warn, and go at 1.0. `padRaw()` is deprecated in favour
  of `padHeld()` rather than renamed: the NMI handler replaces any word that
  is not a joypad's by 0 before either reads it, so the two return the same
  value and a new name would have been a third spelling of it.
  `MIGRATING_FROM_PVSNESLIB.md` no longer offers it as the raw read.
- refactor(lib,examples,docs): **the duplicate names are deprecated** (API
  decision D5). `getRegion()` gives way to `isPAL()` (the same value since
  v0.44), `profileGetFrameCount()` to `getFrameCount()` (the same counter),
  `BGMODE_MODE0/1/2/3/7` to `BG_MODE0`-`BG_MODE7`, and the layer bits
  `WINDOW_BG1`-`WINDOW_OBJ`, `COLORMATH_BG1`-`COLORMATH_OBJ`,
  `MOSAIC_BG1`-`MOSAIC_BG4` to `LAYER_BG1`-`LAYER_OBJ`: one set of layer
  names for `setMainScreen`, `windowEnable`, `colorMathSetLayers` and
  `mosaicSetLayers`. What only one module has keeps its name
  (`WINDOW_MATH`, `WINDOW_ALL`, `COLORMATH_BACKDROP`, `COLORMATH_ALL`,
  `MOSAIC_BG_ALL`), and `TM_*` stays as the register's bit names. Old names
  build and warn; they go at 1.0. A deprecated constant is a macro, so the
  warning comes from `#pragma clang deprecated` in the header, read by the
  clang pre-pass (`-Wno-error=deprecated-pragma` in `make/common.mk`); the
  library itself is linted without that allowance, so it cannot use one.
  Three doc fixes ride along: `mode7SetPivot()` was described as taking
  screen coordinates when it writes the same two registers as
  `mode7SetCenter()` from two `u8`; `docs/tutorials/graphics.md` called
  `setMode()` with one argument; `VBlankCallback` and `VoidFn` now say they
  are the same type.
- **BREAKING** refactor(lib): **no lib variable with an unprefixed name is
  exported any more, and the header functions that needed them are no longer
  `inline`** (API decision D4). `cursor_x` / `cursor_y`, `cgwsel` / `cgadsub`,
  `force_blanked` / `current_brightness`, `sine_table`, `ease_quad_table`,
  `hdma_wave_speed`, `mosaic_size` / `mosaic_bg_mask` and the raw `scope_*`
  words leave the public headers (and take their module's prefix where they
  had none: a game defining `sine_table` or `cursor_x` collided with the lib
  at link time, as `sa1_starfield` nearly did). `fixSin`, `fixCos`,
  `textSetPos`, `setScreenOn`, `setScreenOff`, `getBrightness`,
  `colorMathInit`, `colorMathSetLayers`, `colorMathDisable`, `mosaicInit`,
  `hdmaWaveSetSpeed`, `scopeCalibrate`, `scopeSetHoldDelay` are ordinary
  functions. Cost: `fixSin` / `fixCos`, the only ones called per frame, are in
  assembly and a call costs about 96 master cycles more than the inlined
  lookup did (`backgrounds/mode2`, 32 calls a frame: 17.60 % to 18.46 % of the
  frame; `docs/PERF.md`). Read state through the getters (`textGetX()`,
  `getBrightness()`, `mosaicGetSize()`...).
- refactor(lib): **`easeInQuad()` / `easeOutQuad()`** (API decision D3) replace
  `ease_in_quad()` / `ease_out_quad()`, which stay as deprecated aliases until
  1.0. `sqrt16`, `atan2_8` and `mul16` keep their names.
- **BREAKING** refactor(lib): **the object engine's exported globals are
  gone** (API decision D4). `objgetid` becomes `objGetCurrentId()`,
  `objtokill = 1;` becomes `objKillCurrent();`, and `objptr` was already the
  return value of `objGetPointer()`. Assembly reads `obj_current_id`,
  `obj_kill_flag`, `obj_ptr`.
- **BREAKING** refactor(lib): **the map camera is read with
  `mapGetCameraX()` / `mapGetCameraY()`; the exported globals `x_pos` and
  `y_pos` are gone** (API decision D4, the one row no alias could fix after
  the freeze: a public header must not claim names a game wants for itself).
  Replace `x_pos` by `mapGetCameraX()` and `y_pos` by `mapGetCameraY()`;
  assembly reads `map_cam_x` / `map_cam_y`. Five examples, two manifests and
  the map and object tutorials migrated.
- test(luna-test): **luna v1.31.0.** `luna test` manifests take
  `region = "pal"` (the documented name of `force_region`, still accepted)
  and `--report json` echoes it. `make test-pal` uses `region`. `make tests`
  and `make test-pal` green, no baseline moved.
- test(luna-test): **luna v1.30.4.** Only its MCP server changes (the
  fields Claude Code 2.1.287 sends); emulation is v1.30.3's. `make tests`
  green, no baseline moved.
- test(luna-test): **luna v1.30.3.** Its one fix, a save for Super FX
  cartridges with a battery, changes nothing in our ROMs: our Super FX header
  says `$13`, no battery. `make tests` green, no baseline moved.
- **BREAKING** fix(lib): **`mode7SetScale(0x0100)` is now 1:1**, as its
  documentation always said. `mode7SetAngle()` wrote a matrix of half the
  scale, so `0x0200` was 1:1 and `0x0100` magnified twice, and
  `mode7Init()` + `mode7SetAngle(0)` turned the identity into ×2. The scale
  is doubled before the multiply now (valid up to `0x3FFF`);
  `mode7Transform(deg, 100)` is 1:1 too. **Migration**: halve the scales
  you pass (`0x0200` → `0x0100`); code that relied on the default scale
  after `mode7Init()` and wants the old ×2 view sets `mode7SetScale(0x0080,
  0x0080)`. The four examples that use it were migrated, pixel-identical
  (`diff_corpus`). `mode7Init`'s doc now also says it leaves the view on
  plane row `$180`.

## [0.47.0] — 2026-10-02

The Super FX release. A game can now run while the GSU works: code runs
from RAM (`RAM_CODE_SIZE`, `RAM_CODE_SECTION`, C `RAM_CODE`), jobs run from
the GSU's code cache while the CPU keeps the ROM, a GSU program can have
several entry points (`<name>.sfx.h`, `gsuCall`), and finished frames are
double-buffered and moved to VRAM by the NMI (`gsuPresent`). The new
`chips/superfx_game_skeleton` plays a 60 fps game loop with music while the
GSU renders at 30. Checking that pipeline with luna found `superfx_3d`
losing a third of its framebuffer to a silent VRAM failure; every example
is now held to no VRAM DMA outside blank. Also a `tile` module for tiles
made at run time, and luna v1.30.2.

### Added
- feat(lib): **`tile` module — tiles made at run time.**
  `tileEncode2bpp()`, `tileEncode4bpp()`, `tileEncode8bpp()` turn 64 colour
  indices (one byte per pixel, row by row) into the planar tile the PPU
  reads. Four examples (`mode2`, `game_skeleton`, `panel_hud`,
  `sprite_swarm`) carried the same 4bpp loop as their own copy; they use the
  module now, with the same frames. The encoder is asm: ~47 000 master
  cycles a tile against ~310 000 for the C loop (luna profile). The lib
  fixture checks the three depths and one hand-derived vector.
- feat(lib): **Super FX frames presented by the NMI, double-buffered**
  (superfx runtime chantier, phase D). `gsuPresentInit(vram_a, vram_b,
  flags)` sets up two framebuffers in Game Pak RAM and two char blocks in
  VRAM for `gsu_scmr`'s height and depth; `gsuPresent()` queues the frame a
  job just drew and switches `gsu_scbr` to the other buffer. The NMI moves
  the queued frame to VRAM with what is left of each VBlank — measured from
  the V counter, extended into the top letterbox — taking Game Pak RAM from
  the GSU for each piece (SCMR RAN, Nintendo manual Book II 5.3), and swaps
  BG1's char base only once the whole frame has landed. A 16 KB frame moves
  in two VBlanks with a 40 + 40 letterbox, one with 84 + 4.
  `GSU_PRESENT_ON_LAG_FRAMES` lets a game that never touches VRAM outside
  the NMI have frames move on lag frames too. `gsuSetupBitmapTilemap()`
  follows `gsu_scmr`'s height (128, 160, 192 lines), `gsuStartCached()`
  sets R8 like `gsuLaunch()`, and `gsuCacheLoad()` is asm (the C loop took
  a third of a frame).
- feat(examples): **`chips/superfx_game_skeleton`** — a crosshair steered at
  60 fps, and SNESMOD music, while the GSU renders the cube from its code
  cache at 30, frames presented by `gsuPresent()`. Its manifest proves the
  game never skips a frame during GSU work and that the music plays (luna
  audio RMS); the audio oracle hashes its WAV.
- test(luna-test): **`vram_dma_blank.py`** in `make tests`: every example's
  VRAM DMA lands in blank or force blank (luna's `[asserts.dma]
  unsafe_writes = 0`, one generated manifest per example; 85/85 clean), and
  `gsuPresent`'s frames move whole, into alternating blocks, with every
  BG12NBA swap after a complete frame (luna `--dma-trace`, `--trace-writes`).
- feat(build,lib): **GSU programs with several entry points.** The build
  writes `<name>.sfx.h` next to each `.sfx.bin`: one `#define` per global
  label, its offset in the binary (`gsu_job.sfx`'s `mul_job` becomes
  `GSU_JOB_MUL_JOB`). `gsuCall(entry)` launches at one of them (from ROM,
  like `gsuLaunch()`), `gsuStartCached(entry)` from the cache; arguments go
  in `REG_GSU_Rn`. The GSU fixture runs a multiplication entry both ways.
- feat(build): **code that runs from RAM.** `RAM_CODE_SIZE := N` in a
  Makefile opens a window of N bytes at the top of WRAM bank `$7E`; assembly
  written in `RAM_CODE_SECTION "name"` is stored in ROM bank 1, linked at its
  `$7E` address and copied there by crt0 at boot. It is what a game needs to
  keep running while a Super FX job owns the ROM: the GSU fixture waits on a
  ROM-resident job from the window (33 frames, no bus violation; the same
  loop in ROM loses the CPU). Checked on LoROM, HiROM, SA-1 and FastROM.
  **C functions too** (2026-09-29): `RAM_CODE` (`__ramcode`, a function
  specifier in cproc; QBE emits the window section, with an assembler
  `.FAIL` naming the function when the project has no window). The GSU
  fixture's fourth job waits in C from RAM, 0 bus violations; the same
  function without `RAM_CODE` counts 49 585. `gsu_owns_cart` and `REG_CFGR`
  are now in `superfx.h`.
  `symmap.py` no longer counts wlalink's `RAM_USAGE_*` markers as bank-$00
  ROM (a RAM section at the top of `$7E` read as a full bank $00).

### Changed
- test(luna-test): **luna v1.30.2.** Its `--jobs` keeps manifests chained by
  a battery file in order, so the five power-cycle manifests (SRAM, SA-1
  BW-RAM) are back in `manifests/`, in the one parallel batch; the serial
  `power_cycle/` pass is gone. `[asserts.dma]` no longer stops at a million
  trace events, so `vram_dma_blank.py` checks the Super FX examples over
  the same 200 frames as the rest. No baseline moved.
- refactor(runtime,lib): **the Super FX interrupt entries and `gsuLaunch`'s
  wait loop live in the RAM code window** (a Super FX build reserves 256
  bytes for the SDK on top of `RAM_CODE_SIZE`). They were position-
  independent blobs copied by hand: 160 bytes at boot for the NMI/IRQ/RTI
  entries, 128 bytes of bank-0 RAM re-copied at every `gsuLaunch()` call.
  Now ordinary linked code; `superfx_3d` gains 128 bytes of plain C RAM and
  loses a copy loop per job. Same frames (`diff_corpus` 84/84).

- test(luna-test): luna v1.28.1. The Super FX cache-job fixture now checks
  the job's last RAM write too: luna before 1.28.1 dropped it (a stopped
  GSU no longer clocked its RAM write buffer), a console does not; the
  tutorial's warning about it is gone.
- refactor(examples): **`sa1_starfield` runs its SA-1 code from I-RAM**,
  copied there at boot: measured on luna, ~10.7 MHz with 0.4 % of clocks
  lost to bus conflicts, against ~8.6 MHz and 20 % from ROM, the same frames
  drawn. `make clean && make SA1_CODE_IN=ROM` builds the ROM version; the
  SA-1 tutorial shows both profiles.

### Fixed
- docs: **two hardware claims corrected after arbitration.** The
  `fixMul()` NMI-callback warning no longer says the multiplier returns
  garbage during the auto-joypad read: no reference states it, and a
  main-thread probe on luna v1.30.2 reads every product right during the
  read (19 of 19); the hazard that stands is non-reentrancy. And Mode 5's
  columns: the sub screen draws the even ones, the main screen the odd ones
  — `mode5_hires`'s comment and the graphics tutorial had them inverted
  (snesdev-wiki's Backgrounds page carries the same error). The hardware
  claims rule now reads `snes_verify`'s evidence sentences, never its
  verdict.
- fix(lib): **`superfx_3d` dropped a third of its framebuffer bytes.**
  `gsuDmaFullFrame()` read OPVCT (`$213D`) once per poll iteration and never
  read STAT78, the only reset of OPVCT's read-twice flip-flop: a poll that
  ended on an odd count left the next one reading the high byte, whose bits
  1-7 are PPU2 open bus — the previous exit value, 184 — so the DMA started
  at once (snesdev-wiki; anomie, fullsnes; luna memory trace: 1 281 reads,
  then 1 read returning `$B8` at line 8). luna `--dma-trace` counted
  481 866 of 1 359 872 bytes written on visible lines, silently lost; half
  the transfers began at lines 6-12. It now reads STAT78 and OPVCT twice, starts only in a window from
  which the frame lands whole (225 - bottom to 152 + top) or waits for the
  next frame's, and writes the VRAM/DMA registers after the wait (an NMI
  in it could move VMADD). `gsuSetupHdmaBlanking()` returns only once its
  bands are on screen: HDMA starts at line 0, and the first DMA after it
  found no bottom band (3 417 more bytes lost). `superfx_3d` shows about 30
  whole frames per second instead of 53 partly lost ones; every example
  now passes luna's `unsafe_writes = 0` (`vram_dma_blank.py`).
- fix(runtime): **a Super FX cart could take a VBlank before its WRAM
  vectors existed.** crt0 enabled the NMI, then installed the `$0100-$010F`
  stubs the header's vectors point at; a VBlank in between would have
  jumped into empty RAM. The Super FX init now runs before the NMI is
  enabled.
- fix(compiler): **HiROM symbol files list RAM sections in their real bank.**
  wlalink added `.BASE $C0` to the `[ramsections]` block (a `$7E` section
  read `13e:`, bank-0 RAM `c0:`), so `symmap.py`'s far-RAM-band report saw
  none of a HiROM build's sections. wla-dx patch `8077133` (4 local patches
  now); no ROM byte changes. The far-band report also measures the free
  space below the RAM code window instead of above it.
- fix(runtime): **a Super FX job started with its IRQ on STOP unmasked no
  longer locks the CPU.** With CFGR bit 7 clear and the I flag clear (any
  game that uses a timer IRQ), the GSU's IRQ went to the user's IRQ handler,
  whose `$4211` read does not reset it, and the CPU re-entered the IRQ
  forever. crt0's WRAM IRQ entry now reads SFR bit 15 first, acknowledges the
  GSU's IRQ and counts it in the new `gsu_stop_irqs`: a game can wait for the
  end of a job on that count instead of polling (`CFGR_IRQ_MASK`,
  `CFGR_FAST_MUL` added to `superfx.h`). The GSU fixture runs a second job
  that way.

## [0.46.0] — 2026-09-27

The compiler release. Stack frames are two thirds smaller (temps whose lives
never overlap share a slot), and two emission-time checks now stop the
compiler on the bug classes that produced its silent miscompiles. A Super FX
job can run while the game keeps running. Two library fixes move sprites and
HDMA to where they belong, every public function is documented, and the test
suite runs in under four minutes.

### Added
- feat(lib): **a Super FX job can run while the game keeps running.**
  `gsuCacheLoad()` puts a GSU program of up to 512 bytes in the GSU's code
  cache, `gsuStartCached()` starts it and returns at once, `gsuBusy()` /
  `gsuWait()` follow it. From the cache the GSU does not need the ROM, so
  the CPU keeps running its game loop (`gsuLaunch()` parks it in WRAM for
  the whole job). New fixture `devtools/libtests_gsu`: seven game frames
  during a seven-frame job, no bus violation. Constraints and one open
  question on luna in the Super FX tutorial.

### Fixed
- fix(lib): **sprites drawn by the dynamic sprite engine sit on the line
  you asked for.** `oamSet()` and `oamDrawMeta()` store y − 1 (the PPU
  draws an OAM Y of N from line N+1); the dynamic engine
  (`oamDynamic*Draw`, `oamDynamicMetaDraw`) stored y itself, so the same
  y drew one line lower. Five examples move their sprites up one line:
  dynamic_sprite, dynamic_metasprite, slope_collision, mapandobjects,
  likemario.
- fix(lib): `gsuSetupHdmaBlanking()` no longer switches off every other
  HDMA channel, nor gets switched off by the next `hdmaEnable()` /
  `hdmaDisable()`: it arms its channel through the `hdma` module's record
  of `$420C` (write-only) instead of writing the register. The `superfx`
  module now depends on `hdma`.
- fix(luna-test): project tests (`make test`) run on Windows. The harness
  passed `--out /dev/null` to luna, which `luna.exe` cannot open; the
  failure was reported as "all assertions pass" next to a FAIL. It now
  uses the platform's null device and reports luna's own error.
- fix(build): a zip built from `develop` is named after the tree's
  version (`snes.h`), not after v0.17.0 (the last tag `git describe`
  reaches from develop).

### Tests and documentation
- test: `make tests` runs its luna calls in parallel (3 min 41 on 6 cores,
  byte-identical reports); corpus liveness takes a second snapshot, so an
  NMI that dies after boot no longer passes; the harness has its own unit
  tests; every compiler fixture now asserts its code (41 ported, ratchet at
  0); six examples' controls are scripted and asserted; the c_features ROM
  calls functions returning 32-bit values.
- test: luna v1.28.0 — manifests name array elements as `symbol+N`, an
  empty controller port is tested one port at a time, and the SA-1 speed of
  `sa1_starfield` is measured (~8.6 MHz; tutorial updated).
- docs: the 81 public functions no page mentioned are documented (new
  interrupts tutorial); `docs/tools/build.md` lists every Makefile knob, and
  the doc sentinel fails on one it does not name (`BPP`, read by nothing, is
  gone); `MAINTAINING.md` says what a successor needs; `compiler/ABI.md` no
  longer says 32-bit values return through the stack.

### Changed
- perf(compiler): **stack frames are two thirds smaller.** Temps whose lives
  never overlap now share a stack slot (slot colouring from liveness, qbe
  `794c6e3`). Over the examples: median frame 38 → 16 bytes, largest 518 →
  214, none past 256 any more (six functions were, in the slower
  large-frame addressing), 16 776 → 5 826 bytes in total; the deepest stack
  of the library fixture leaves 880 bytes above the C variables instead of
  572. An emission-time check stops the compiler if a temp is read from a
  slot another temp overwrote. Programs compute the same values: the ROMs
  render identically, except two whose loops now finish more work per frame
  (sprite_swarm starts one frame sooner, mode2 misses two frames fewer).
- refactor(examples): **`hdma/hdma_wave` and `hdma/hdma_wave_table` are one
  example** (decided 2026-09-05): the hand-built table animated krom-style
  at boot, pixel-identical to the old `hdma_wave_table` at both capture
  points, then A hands the ripple to the `hdma` module's `hdmaWaveH` with
  LEFT/RIGHT for the amplitude. The corpus goes from 85 to 84 examples.
- fix(compiler): four upstream cproc fixes and one QBE fix are absorbed
  without a resync: character constants with a hex or octal escape above
  0x7F (`'\xFF'`), overflow while parsing escapes, overflow in array
  growth, and QBE's exponential time in `usewidthle()`. Every ROM of the
  corpus is byte-identical; the forks' own suites pass with no new
  known-fail.
- feat(compiler): **the emitter checks the class of bug that produced four
  silent miscompiles in three months** — a 32-bit value whose high word no
  instruction wrote. Every high-word read now requires a prior write, or
  compilation stops with an internal compiler error naming the value and
  the function (`QBE_KL_CHECK_WARN=1` lists every hit instead). A copy of
  a 32-bit value now moves both words. 0 hits over the library, every
  example and the compiler fixtures; ROMs byte-identical.

## [0.45.0] — 2026-09-26

The state-of-the-project release. Eight audit agents read the SDK aspect by
aspect on 2026-09-26; this release is what their first wave fixed. The two
headline fixes are for users: **the release zip could not build a project**
(every zip since July), and **SNESMOD's stop and pause sometimes left a
note sounding** (a bug of the original driver, shared with PVSnesLib). The
Super FX gets interrupts that keep working during GSU jobs, the build
refuses the knob combinations it used to build wrong, and the docs stop
citing functions that do not exist.

### Fixed
- fix(build,ci): **the release zip builds a user project again.**
  `make/common.mk` has run `devtools/check_bank_reads.py` after every link
  since July, and `make release` did not ship it, so the zip's starter
  failed at its first `make`. The zip now carries every script `common.mk`
  runs (read from `common.mk` itself), ships the starter without build
  output, and CI extracts the zip and builds and tests a project from it
  on all four OS at every push (`make release-smoke`).
- fix(lib): **SNESMOD stop and pause no longer leave a voice sounding.** The
  driver cleared KOFF about 60 SPC cycles after setting it, under the
  S-DSP's 64-cycle poll, so a key-off was sometimes missed (8 times in 161
  on luna). One instruction moved into the gap, same size: 0 in 161.
  Music started this way also begins two DSP polls earlier.
- fix(runtime,lib): **`padIsConnected()` tells an empty port from a pad.**
  It answered 1 for an empty port; the NMI handler now reads the 17th
  serial bit of each pad port.
- fix(runtime): **Super FX: interrupts keep working during a GSU job.** The
  interrupt vectors live in WRAM, where the GSU sends them (Nintendo dev
  manual Book II §5.4.1): the NMI counts the frame and uploads OAM while
  the GSU owns the cartridge (one VBlank in three was lost before), an
  H/V-timer or GSU IRQ is acknowledged instead of running garbage from
  ROM, BRK and COP return. `gsuLaunch()` no longer touches `$4200`.
- fix(build): the build **refuses** two coprocessors at once, Super FX /
  SA-1 / DSP-1 with HiROM, SRAM with Super FX, `SRAM_SIZE` outside 1..7 and
  a non-numeric `ROM_BANKS` — each with its reason. DSP-1 + SRAM declares
  `$FFD6 = $05` (was `$03`); `ROMSIZE` is rounded up. A knob changed after
  the first build (`USE_FASTROM`, `USE_SRAM`, `ROM_BANKS`…) is no longer
  ignored; an unknown `LIB_MODULES` name is an error listing the modules;
  local headers are dependencies; the lib and the tools depend on their
  headers and included files.
- fix(build): the NMI / WRAM-port race lint runs again. It only ran when a
  `combined.asm` existed — a file the build stopped producing long ago.
- fix(tools): `smconv` refuses a file that is not an Impulse Tracker module
  (a PNG used to build a silent soundbank); two allocation bombs the fuzzer
  found (IT table counts, lodepng's pre-inflate reservation).
- fix(examples): the object engine read the map from bank $00 whatever bank
  it was in; examples' assets left bank $00 (`ASSET_SECTION`), bank $00
  minimum across the corpus 12 → 1912 bytes free.

### Added
- feat(lib): `gsuSetProgram(const void *program)` — `superfx.h` asked for it
  but only an example defined it.
- feat(build): `ROM_BANKS` and `GSU_RAM_KB` knobs; the Super FX header
  declares its Game Pak RAM (`$FFBD`, `$FFDA = $33`).
- test: luna pinned at v1.27.0: a measured stack gate on every coverage leg
  (the stack must never reach the C variables), mouse / Super Scope /
  pad 2 replayed in the coverage, `[asserts.gsu] bus_violations = 0` on the
  Super FX manifests, `padIsConnected` tested with both ports unplugged.

### Changed
- build(compiler): the cc65816 retry on a cproc segfault is retired (zero
  firings in 25 Windows builds); a crash is now reported, not absorbed.
- ci: the 50-minute fuzz session runs when `tools/` changes and weekly;
  scheduled workflows test `develop`, not the last release.
- docs: `BENCHMARK.md` re-measured — about **20 %** faster than PVSnesLib +
  816-opt on 33 functions, slower on pointer-heavy code since 4-byte
  pointers (it said 32 %). `ABI.md` documents the 32-bit return
  convention. Tutorials and tool pages no longer cite
  `colorMathSetMaskMain/Sub`, `objRegisterTypes`, `spcLoad/spcPlay`; the
  sentinel (`make lint-docs`) now fails on an SDK-shaped call no header
  declares and on a deprecated name cited as current.
- docs: `ATTRIBUTION.md` lists cmdparser (Apache-2.0, licence shipped in
  the zip), stb_image and cute_tiled.

## [0.44.0] — 2026-09-22

The audit release. Every public function of the SDK is now executed by a
test — 311 of 311, from 167 "never executed" at the start of the cycle —
and an API audit read all 36 headers against their implementations before
the v1.0 freeze. What the two found is the release: the audit's 20 verified
defects, 19 of them fixed here, and a dozen more the tests turned up —
including two silent miscompilations and an arctangent table that was a
sine. Plus a naming pass that gives one meaning to each word while the
aliases are still free, and one breaking change, `TRUE`, because a value
cannot be aliased.

### Breaking
- **`TRUE` is 1, and every predicate returns 0 or 1.** `TRUE` was `0xFF`, a
  PVSnesLib habit, and three functions returned it (`isPAL`, `isInVBlank`,
  `padIsConnected`) while every other predicate returned 1 — so
  `isPAL() == 1` was false on a PAL console and `x == TRUE` failed on the
  majority. `getRegion()` and `isPAL()` are now the same value. Code that
  compares with a literal `0xFF`, or uses `TRUE` as a mask, must change;
  `if (pred())` and `!pred()` — the documented idiom — never noticed
  either value.

### Deprecated
Thirteen functions and one macro keep working with a compile-time warning
and are removed at the next major. Renamed so that one word means one
thing:

| Deprecated | Use instead | Why |
|---|---|---|
| `scopeButtonsDown` | `scopeButtonsHeld` | "Held" meant the opposite of `padHeld` on the scope; the auto-repeat mask is now `scopeButtonsRepeat` |
| `colorMathEnable` | `colorMathSetLayers` | it REPLACES the layer set — "Enable" reads as additive, and `windowEnable` is |
| `mosaicEnable` | `mosaicSetLayers` | same |
| `sa1Init` | `sa1IsReady` | it never initialised anything; crt0 boots the SA-1 before `main()` |
| `dsp1Present` | `dsp1IsPresent` | consistency with the other chip getters (`gsuIsPresent` is new) |
| `LzssDecodeVram` | `lzssDecodeVram` | the one capitalised function of the SDK |
| `rand` / `srand` | `rngNext` / `rngSeed` | libc names for a generator that is not libc's |
| `nmiSetBank`, `irqSetBank`, `dmaCopyVramBank`, `dmaCopyCGramBank`, `hdmaSetupBank`, `OAM_SET_GFX_BANK` | `nmiSet`, `irqSet`, `dmaCopyVram`, `dmaCopyCGram`, `hdmaSetup`, `OAM_SET_GFX` | the plain forms read the bank from the pointer (chantier A6); the explicit-bank forms predate it |

`scopeButtonsHeld` is the one behaviour change: it now means "currently
down". A caller that wanted the auto-repeat mask must move to
`scopeButtonsRepeat`.

### Fixed
- fix(compiler): **on HiROM, every C pointer to a RAM variable carried a
  ROM bank.** HiROM units assemble under `.BASE $C0` and wlalink added
  that base to a RAM label's bank, so `pea.w :var` pushed `$C0` — ROM on
  HiROM. Any routine honouring the bank byte of a pointer it was handed
  read or wrote ROM instead of work RAM. LoROM was unaffected. Fixed in
  the wlalink fork; found by the first HiROM library fixture.
- fix(compiler): **the address of a local variable had an undefined
  bank.** `&local` is a far pointer, but the backend stored only its
  16-bit stack address and left the bank half holding whatever the stack
  had in that slot. Harmless for a bank-blind dereference, wrong for every
  reader that honours it — a `const T *` parameter, a library routine
  reading the bank byte. It survived because an unused stack slot reads 0
  after power-on, which is the stack's own bank.
- fix(lib): **`atan2_8()`'s lookup table was not an arctangent** — it
  tracked a sine, up to 7° off mid-octant. The axes and the diagonal,
  which every test had checked, were right. Regenerated; one mid-table
  vector found it.
- fix(lib): **`sramSave`/`sramLoad` used the LoROM address on every
  build.** HiROM battery RAM is at `$30-$3F:6000`, not `$70:0000`, so a
  HiROM save went to open bus and loaded garbage.
- fix(lib): **`snesmodFlush` crashed whenever it had something to flush**
  — it reached the SPC message pump through a `jsr` into a routine that
  ends in `plb`/`plp`/`rtl`.
- fix(lib): **five functions were handed a far pointer and dropped its
  bank** (`irqSet`, the LZSS decoder, `dmaCopyOam`, the SRAM block copies,
  `OAM_SET_GFX`) — a table outside bank $00 was read from the wrong one.
- fix(lib): **three argument bugs that symmetric test inputs were
  hiding** — `snesmodFadeVolume` read the wrong stack slot,
  `gsuSetupHdmaBlanking` had two arguments swapped, `hdmaColorGradient`
  emitted a malformed table — plus `snesmodPlayEffect` not masking its
  packed fields. Equal or zero values for distinct parameters is why they
  had survived; every new vector uses distinct ones.
- fix(lib): **`objCollidObj` returned garbage on its first call**, and the
  object workspace wrote back list links over `onscreen`. Plus a pool leak
  in `objKillAll`, unguarded null callbacks, and the documented scene-swap
  idiom leaking a stack slot per lap.
- fix(lib): **the five object routines that take a slot index now accept a
  handle.** They shifted the argument with no mask, so the natural call —
  with the handle `objNew()` returned — put the id byte into the buffer
  offset and worked on memory past the pool.
- fix(lib): **a `u8` sprite id of 256 silently overwrote sprite 0.** The
  truncation happened before the range check could refuse it. Sprite ids
  and coordinates are `u16` everywhere now (same stack slot, no ABI
  change), `fixLerp`'s `t` is a `u16` so the documented 1.0 is reachable,
  and `dsp1Multiply` is signed.
- fix(lib): nine more verified defects — a two-pass `dmaFillVRAM`,
  `nmiSet(NULL)`, `fix32Div` by zero, a `mapLoad` clamp, an
  `hdmaWaveH` clamp, an `oamMetaDrawDyn` bound, the profile module, the
  SA-1 CCNT constants.
- fix(lib,docs): two DSP-1 commands contradicted their own documentation,
  which running the real firmware settled: `Distance` reads one low on
  exact lengths ((3,4,12) gives 12), and `Range` returns the squared
  difference shifted right by 15, not the raw difference. No hardware
  reference we could find states either.

### Added
- feat(lib): **the audio and SRAM APIs report their failures.**
  `audioInit` and eleven setters return their `AUDIO_*` code instead of
  dropping it; the five SRAM copy functions return `SRAM_OK`,
  `SRAM_ERR_RANGE` or `SRAM_ERR_NO_SRAM` and a refused transfer copies
  nothing — the capacity is read from the ROM's own header.
- feat(lib): **`audioPlaySampleOn(voice, ...)`** — the caller picks the
  voice, so an envelope can be aimed before key-on and a long sound cannot
  be stolen by the eighth effect. `AUDIO_VOICE_AUTO` finally has a
  function to be passed to.
- feat(lib): **`sceneReplace()`** — the documented `scenePop(); scenePush()`
  swap cannot replace the bottom scene, so a title → game → title cycle
  leaked a slot per lap until the stack silently dropped pushes.
  `scenePush` / `scenePop` now return 1 or 0, and `objGetPointer` returns
  the slot instead of reporting failure through a global.
- feat(lib): **`const` on every read-only pointer parameter** — on this
  target that is correctness, not style: const data lives in the asset
  banks and only a const-qualified load is a far read.
- feat(docs,build): **a hardware verification protocol** — 22 ROMs, one
  check each, and `make hardware-kit` to collect them for a console
  session. Claims that no reference settles are marked as such.
- feat(luna-test): **ROM coverage is measured, not assumed** — each
  manifest's input script is replayed through `luna profile --pc-set` and
  the library fixtures are counted. 311 of 311 public functions execute;
  the ratchet fails a new one.
- feat(devtools): **four library fixtures, 282 runtime assertions** —
  the main ROM, a second for HDMA/Mode 7/SNESMOD, a firmware-gated DSP-1
  one, and a HiROM one that proved the pointer-bank defect above.

### Changed
- chore(devtools,ci): Dependabot's action bumps applied, targeting
  `develop`.
- docs: the header documentation that described designs which no longer
  exist — twenty headers and `compiler/ABI.md` — now describes the tree as
  it is. The "coordinates MUST live in an `s16` struct" warning on
  `oamSet` is gone: it described a compiler defect of early 2026, and the
  `move_sprite` manifest pins plain-`u16` motion to the pixel.

### Known
- `padIsConnected()` answers 1 for an empty port. Auto-joypad reading
  cannot distinguish an empty port from an idle pad; the difference is
  past bit 16 of a manual serial read. The fix needs a luna release that
  can model an unplugged port.
- The `superfx` module cannot be linked into a LoROM ROM: its object
  force-emits `gsuInit`, which references state defined in a SuperFX-only
  object. It is auto-added under `USE_SUPERFX=1`, where it works.

## [0.43.0] — 2026-09-17

The instrument release. The 2026-09-11 gaps review's whole prioritised
backlog ships here — every tier, 42 of its 43 items — and what matters is
not the instruments themselves but what they caught the first time they
ran: a save-erase routine that wrote a byte ramp instead of zeros, five
silent miscompilations, two heap overflows and two allocation bombs in
vendored parsers, and seven latent host bugs. Functional test coverage of
the example corpus went from 35 of 85 to 84 of 85. The one item still open
(a per-frame VBlank time budget) waits on luna folding the NMI handler's
child profile rows into their parent.

### Added
- feat(devtools,ci): **the host toolchain runs under ASan + UBSan**
  (`make test-sanitizers`, `SANITIZE=1`, CI job). Its first run found
  seven latent bugs in five programs, including a QBE use-after-free and
  a wla-65816 read before its token buffer.
- feat(devtools,ci): **the three submodules' own upstream suites run on
  the fork binaries** against known-fail ratchets
  (`make test-toolchain-suites`) — the check every PIN bump was missing.
- feat(tools,ci): **libFuzzer harnesses for every asset parser**
  (`tools/fuzz`, `make fuzz`, `make fuzz-replay`): lodepng, smconv's
  Impulse Tracker loader, cute_tiled, the Aseprite JSON parser and
  stb_image. Crashing inputs are committed and replayed on every push.
- feat(devtools,ci): **compiler host coverage** (`make coverage-host`,
  clang source-based, CI artifact). First measurement: 72.8 % of lines
  over the fork-owned files, 84.1 % over the w65816 backend.
- feat(luna-test): **an audio-output oracle** — `audio_regress.py` hashes
  300 frames of APU output for four self-playing examples, in `make tests`.
- feat(luna-test,ci): **a PAL pass** (`make test-pal`, weekly workflow):
  the corpus booted at 312 lines and 50 Hz, plus `getRegion()` / `isPAL()`
  asserts on the library fixture.
- feat(luna-test,ci): **nightly `luna bench`** over the whole corpus
  (`make luna-bench`), and `--native-res` capture for the hi-res example.
- feat(devtools): **`make test-link-modules`** links every library module
  alone and together — it found seven undeclared dependencies.
- feat(devtools): runtime assertion ROMs for **C features** (64 asserts)
  and for **collision, SRAM, the raw IRQ path, the window module and the
  region getters** (the library fixture now carries 99 vectors).
- feat(docs): **a PVSnesLib migration guide, an FAQ and a profiling
  tutorial**, and a documentation index that finally lists all 24
  tutorials, the craft guides and the tool pages — with a measured
  header-to-tutorial map naming what is still undocumented.
- feat(luna-test): **107 manifests, covering 84 of the 85 examples**
  functionally (was 35). The seven games and the input examples answer
  their pad; the rest assert what they demonstrate — VRAM and CGRAM
  destinations, S-DSP state before and after an effect, HDMA channel
  shadows, the Mode 7 matrix, OAM decoded through OBJSEL.
- feat(ci): supply-chain hygiene — every action pinned to a commit SHA,
  Dependabot, an `.editorconfig`, and a Doxygen warnings-as-errors gate
  (`make docs-strict`) after driving 56 warnings to zero.

### Fixed
- fix(lib): **`sramClear()` wrote a byte ramp instead of zeros.** The loop
  compared its index through the accumulator and never reloaded the zero,
  so byte 0 was cleared and every byte after it received its own offset.
  No example called it, so the tutorial's "delete save" path had shipped
  broken; the library fixture's save/clear/load round trip found it.
- fix(compiler): **five silent miscompilations**, found by the new
  C-feature ROM — bit-field reads returning 0, 32-bit shifts by a variable
  count shifting only the low word, signed 32-bit compares, `jnz` on a
  32-bit value, and a sign-extension clobber.
- fix(runtime,lib): vertical scroll writes `y - 1` — the PPU never outputs
  scanline 0, so the library now applies the correction for you.
- fix(compiler): four clang 18 null-plus-zero sites in cproc and QBE that
  only the sanitizer job and the upstream suites reached.
- fix(tools): **two heap-buffer-overflows in cute_tiled** (the whitespace
  scan and the error message both read past the caller's buffer, which
  the parser neither copies nor terminates) — fixed structurally by
  parsing a NUL-terminated copy.
- fix(tools): **two allocation bombs** — smconv's IT loader asked for
  3.7 GB from a sample header, and an 840-byte PNG made stb_image ask for
  2 GB from a declared IDAT length. Both bounded by what the input can
  actually supply.
- fix(tools): undefined behaviour in stb_image's PNG accumulator, and an
  unsigned alpha mask in tmx2snes.
- fix(examples): `hicolor_1792` spilled its CGRAM DMA past H-blank, which
  luna v1.23.0 made visible; the H-timer moved to a measured clean window.
- fix(ci): the Doxygen gate no longer sits in the Doxyfile, where it broke
  release builds on two platforms.

### Changed
- chore(luna-test): **luna pinned at v1.23.0** (per-frame profile rows,
  `--budget`, mid-picture CGRAM modelling, overscan).
- refactor(tools): lodepng and cmdparser live in `tools/common` instead of
  being duplicated byte for byte in two tools.
- chore(devtools): 14 compiler fixtures gained assertions; the unchecked
  ratchet went 55 to 41.
- docs(claude): the structural-defect catalogue, the roadmap and eight
  stale notes now describe the tree as it is.

### Known
- The multitap path is present but **unreachable**: nothing sets the flag
  the NMI handler tests, so pads 3, 4 and 5 are never read. See
  `KNOWN_LIMITATIONS.md`.
- `color/gradient_9bit` has no functional manifest: its effect lives
  entirely during the picture, and a manifest cannot assert a PPU register.

## [0.42.0] — 2026-09-12

The gate release: the harness captures at PPU frames, CI runs the same
`make tests` / `make lint` a contributor runs, the toolchain builds with
warnings as errors, and two new luna passes (random power-on RAM, A/B at
equal frame) start finding what zero-filled emulation could not. luna is
pinned at v1.21.0, which ships the fixes and features requested in the
2026-09-11/12 reports. A written gaps review with a prioritised backlog
drives the next releases.

### Added
- feat(luna-test): **`make tests` runs the corpus liveness pass a second
  time from pseudo-random power-on RAM** (`luna_runner.py --power-on
  random=1`, fixed seed). A ROM that reads memory it never initialised
  passes on luna's zero-fill and fails here — the class of the v0.40.0
  and v0.41.1 reset-vector fixes. First run found nine examples whose
  rendering depends on the power-on state; six are fixed below, three
  (`backgrounds/mode0`, `color/hicolor_blend`, `games/tetris`, last
  scanline only) are still under investigation.
- feat(luna-test): **`diff_corpus.py`, the Class A validation protocol** —
  `luna diff <before> <after> --frames <manifest frames> --tolerance N`
  over every example against the ROMs built before a change; MATCH with
  the boot-length offset, or DIFF with PNG pairs. `testing.md` now requires
  it before any re-baseline of a compiler or library change. First use
  validated the `-O2` toolchain rebuild: 85/85 MATCH at ±0, ROMs
  byte-identical.
- feat(luna-test): `manifests/input_two_players.toml` — the first
  two-controller functional probe (luna v1.21.0 `input2`), both `pad_keys`
  words asserted over three legs.
- test(devtools): the `a7_32bit` runtime ROM gains `r_shl8_ld` (a 16-bit
  value loaded from a table, widened to `s32`, shifted left 8);
  `test_function_ptr.c` covers const, RAM, 2D and struct function-pointer
  tables and gets a `.checks` file (`MAX_UNCHECKED` 56 → 55).
- docs(claude): **`.claude/notes/reviews/2026-09-11_gaps_review.md`** —
  a review of what the project and its AI agent lack on three axes
  (development information, the Cartouche SNES corpus, tooling), 43
  backlog items in three tiers; and `.claude/notes/tech/cartouche_corpus.md`,
  the in-repo list of corpus sources, index fingerprint and golden
  queries behind the hardware-claims rule.

### Changed
- chore(luna-test): **visual baselines are captured at PPU frames**
  (`--until-frame`; `manifest.toml` `default_frames = 200`, self-animating
  examples `frames = [200, 400]`) instead of instruction counts, so a
  codegen change can no longer move a capture onto another animation
  phase. One re-baseline, justified by the old instruction-keyed points
  all still matching first.
- chore(luna-test): **luna pinned at v1.21.0** — fbhash v2 (FNV-1a 64
  over the displayed RGBA, stable by construction), `--input` applied
  under `--until-frame`, `input2`, `luna profile --pc-set`, the
  `power_on`/`seed` pair in `luna test --report json`, `$43xx` readable
  under `--peek`. All 85 baselines re-keyed; 26 sprite-bearing examples
  also changed pixels because v1.18.0 drew every sprite one row too high
  (luna measured v1.20.0 against Mesen2 on the SDK's own ROMs: 100 %
  identical). One WRAM baseline (`chips/superfx_3d`) re-keyed for a
  benign boot-timing shift documented in the commit.
- ci: **the functional-tests job runs `make tests` and the lint workflow
  runs `make lint`, verbatim.** Three linters, two runtime ROM tests and
  the corpus-freshness guard enter CI for the first time; the no-op
  "functional probes" step is gone; the user-project story
  (init → build → test-update → test → FAIL path) is `make test-project`
  inside `make tests`. The cached-toolchain stamp is "now" instead of 2030
  so the freshness guard sees ROMs newer than `bin/cc65816`; the cache key
  hashes `compiler/Makefile` as its comment always claimed.
- build(compiler): **cproc-qbe and QBE build with `-O2` and warnings on;
  `-Werror` on the Linux CI legs** (`TOOLCHAIN_WERROR=1`). cproc fork
  `98ecf20` (18 patches) drops three dead fork-local symbols the flags
  exposed. `tmx2snes` loses `-Wno-implicit-function-declaration`,
  `tmx2snes` and `sa1-patch` gain `-Wextra`. ROMs byte-identical.
- build(lib): `lib/source/*.c` gets the same clang
  `-Wall -Wextra -Werror` pre-pass every example already had.
- ci: the `symmap --check-overlap` step is blocking in the build matrix
  and in `release.yml` (it used to end in `true`).
- docs: `CLAUDE.md` and `ROADMAP.md` state the post-B2 / post-#127.3
  memory model (`FAR` above $2000, C const data in the asset banks) — they
  still described the pre-#127 SUPERFREE spill. `compiler/ABI.md` is the
  arbiter for cc65816 ABI questions in the corpus (documented error on the
  upstream QBE ABI doc).

### Fixed
- fix(examples): **six audio examples cleared VRAM nowhere** (`apu_switch`,
  `echo`, `pitch_mod`, `play_noise`, `soundboard`, `speech_synth`): crt0
  enables BG1 and `consoleInit()` points its tilemap at VRAM $0400, so on
  real hardware the power-on garbage showed through the backdrop. Each now
  calls `dmaClearVRAM()` after `consoleInit()`. Found by the random
  power-on pass. Whether `consoleInit()` should clear VRAM by default is
  an open design question in the review (R10).
- fix(luna-test): user-project tests (`make test` in a scaffolded project)
  broke for one push after the frame-capture rename; restored, and the
  story now runs in `make tests` so it cannot regress unseen.
- docs(claude): two compiler-bug notes said OPEN for bugs fixed on
  2026-08-13 and in v0.21.2 (ternary address-constant bank drop, `Kl`
  shift high half); corrected, with regression coverage added instead of
  KNOWN_LIMITATIONS entries. The luna campaign note listed four luna
  issues as open that closed in v1.16.0.

## [0.41.1] — 2026-09-08

A boot-safety patch: the reset vector silences NMI and HDMA before the
memory clears, on the strength of the corpus arbitration that came back
online.

### Fixed
- fix(runtime): **NMITIMEN and HDMAEN are zeroed at the reset vector**,
  right after the forced blank and before the 8 KB bank-0 clear and the
  22 ms far-band zero-fill. They were reset only in `InitHardware`, and
  the zero-fill assumed `$4200 = 0` as a given. On real hardware, or on
  any emulator that randomises power-on state, an HDMA channel left
  enabled could write junk to the PPU or into `$2180-$2183` mid-transfer.
  Arbitrated: snesdev-wiki "Init code" — "most of the PPU registers start
  in an unknown state ... the first registers to reset should be the
  NMITIMEN and HDMAEN registers"; oldmachines — "VRAM, CGRAM, OAM and WRAM
  hold whatever the silicon happened to power up as". luna zero-fills
  everything and cannot see this class of bug.
- ci: push and pull_request runs of the same commit no longer cancel each
  other (the concurrency group includes the event name); release PRs read
  as mergeable without a manual re-run.

### Changed
- docs(runtime): the HiROM `.BASE $C0` rationale in `memmap_hirom.inc`
  cites the arbitrated mapping (fullsnes: HiROM banks 40h-7Dh with mirror
  at C0h-FFh; snesdev-wiki: the linear view of the entire ROM at $C0-$FF).

## [0.41.0] — 2026-09-07

The placement release: C const data leaves bank $00 by default, HiROM is
addressed in its full 64 KB view, and the linker fork gets its first
patch to make that correct. Plus a bench that finally measures the
const-path gains, and a CI moved to Node 24.

### Added
- feat(compiler): **C const data is placed in the memory map's asset
  banks by default** (#127.3, closes #127). QBE emits every `.rodata.N`
  as `SEMISUPERFREE BANKS ASSET_BANKS` — the directive `ASSET_SECTION`
  already used — so `static const` tables, string literals and const
  structs land in the asset banks (highest first) and bank $00 keeps the
  code. Safe because every C read of const data is a far read (#121),
  every lib call carries a far pointer (A6), and `check_bank_reads.py`
  fails the link on a bank-$01+ symbol read with bank-$00 addressing (0
  hits across the corpus). Bank $00 minimum went from 12 bytes to 71 on
  the code-heavy games, which now spill code (not data) into bank $01+
  the way `jsl` allows. Visual regression 85/85 identical.
- test(devtools): `benchrom` gains seven const-path workloads (animTick,
  animTickMeta, const pointer walks, const→RAM copy, const struct
  fields, `tab[i]`), the instrument the A9 optimiser change lacked:
  v0.39.0 → v0.40.0 measured −9 % to −54 % per call. The runner reads
  every result from one luna run instead of one per symbol.

### Fixed
- fix(build): **HiROM addressed in its full view.** Every HiROM unit now
  carries `.BASE $C0`, so a label at offset $0000 of linker bank *n* is
  addressed at `$Cn:0000` — the only mapping of that half. Until now the
  runtime's `.mul32`/`.div32` sat at `07:0000`, a WRAM mirror: any HiROM
  C program doing 32-bit arithmetic would have crashed, and the 2026-07
  attempt at #127.3 corrupted HiROM for the same reason.
- fix(compiler): **wla-dx fork, first local patch** — `.BASE` no longer
  applies to RAMSECTION labels (`$7E + $C0` was out of 24-bit range;
  `$7E + $80` under FastROM silently gave `$FE`). `symmap.py` and
  `check_bank_reads.py` fold the `$C0`/`$80` window back to the linker
  bank; the `.sym` prints one valid CPU address per line, which luna
  needs.
- fix(lib): `consoleNocashMessage` read its string through a 16-bit
  bank-$00 pointer (pre-A6); it goes through the 24-bit pointer now.
  Caught by the `debug_channel` runtime fixture the moment string
  literals moved.
- ci: the workflows' actions run on Node 24 (checkout v5, cache v5,
  upload-artifact v6, download-artifact v7, action-gh-release v3); the
  deprecation notices are gone.

### Changed
- docs(docs): `KNOWN_LIMITATIONS.md` bank-$00 ROM entry rewritten (code
  only; the one bank-blind path left is an explicit cast that drops
  `const`, guarded at link time); `.claude/rules/bank0_budget.md` records
  the shipped default and retires the "what still blocks it" section;
  `templates/assets.inc` HiROM caveat resolved; the two open hardware
  claims (HDMA mid-frame enable, INIDISP at power-on) are arbitrated
  against the SNES corpus and cited in `hdma.asm` / `crt0.asm`.

## [0.40.0] — 2026-09-07

The optimiser release: const and far accesses were pinned in QBE's
optimiser as if `volatile`; the lib alone loses 4 % of its instructions
now that only the volatile bit pins. Plus a boot fix that every emulator
with a randomised power-on state (and real hardware) could show.

### Performance
- perf(compiler): **the access flag no longer pins const/far loads in
  QBE's optimiser** (chantier A9). `load.c`, `mem.c` and `gcm.c` tested
  the whole access flag, so every const-data read, every `FAR` access and
  every load of a `const T *` / `T FAR *` pointer variable was never
  forwarded, never promoted out of its alloca, never eliminated. The
  three passes now test the volatile bit only, and cproc no longer taints
  a pointer variable's own load with its pointee's qualifiers (`lda.w p`,
  not `lda.l p`). Corpus audit: 285 const loads and 65 far accesses pinned
  by accident against 201 volatile loads. Lib −685 instructions
  (−4.25 %); estimated cycles asset −34 %, anim −17 %, panel −15 %,
  sprite −8 % — allocas holding const walkers are promoted, so a
  `*dest++ = *src++` loop loses its per-iteration round trips. Verified
  frame-identical to v0.39.0 on the whole corpus.
- feat(devtools): `CC65816_KEEP_IR=<dir>` on the `cc65816` wrapper keeps
  every translation unit's QBE IR for corpus-wide audits.

### Fixed
- fix(runtime): **force blank as the first thing after reset**. The first
  INIDISP write happened in `InitHardware`, 40.9 ms after reset once the
  bank-0 clear and the far-band zero-fill ran before it (2.5 frames,
  measured with luna). On Mesen2, which randomises the power-on state, that
  was a visible burst of garbage on first launch (not on "reload ROM"); luna
  zero-fills everything and could not show it. The reset vector now writes
  INIDISP = $8F at its 7th instruction. Reported by the maintainer on the
  v0.39.0 build.

## [0.39.0] — 2026-09-06

The far RAM release: chantier B2 lifts the 8 KB C RAM ceiling. A global
declared `FAR` lives in bank $7E (56 KB) and every access to it is
compiled bank-honouring, at the cost of the bank-0 forms or less.
breakout goes from 1436 to 6880 free bytes in bank 0, pixel-identical.

### Added
- feat(compiler): **`__far` type qualifier, `FAR` in `snes/types.h`** —
  the object is placed in `$7E:2000-$FFFF`; direct accesses take
  `lda.l/sta.l sym`, `sym[idx]` is one absolute-long-indexed instruction
  (the address add is folded away), runtime pointers go through
  `[tcc__r9]` with base+const and base+index decomposed onto `,y` and a
  per-block pointer cache. Rides on the type like `const`: static storage
  only, not `const`, dropping it into a plain pointer is a compile error,
  and `T FAR *` converts to `const T *` freely so the DMA/HDMA/asset API
  takes far buffers as they are. Initialisers work (the boot data-init
  record carries the bank) and the band is zeroed at boot (one DMA,
  22 ms). Cost (`devtools/benchrom/b2_deref`): pointer walks +3/+4 % vs
  bank 0, `arr[i]` −18 %, stores −17 % / −48 %.
- feat(devtools): `symmap.py --check-ram-budget` reports the far band
  (`OK: far RAM band $7E:2000-$FFFF: N bytes free …`) next to the bank-0
  one; runtime fixture `devtools/compiler-tests/runtime/b2_far_ram`
  (25 cells) and `cases/far_ram_forms.checks` pin every far form.
- docs(docs): **`docs/tutorials/far_ram.md`** — the two bands, the rules
  the compiler enforces, the cost table, when to use `FAR` and when not,
  the breakout migration as the worked example. `compiler/ABI.md` gains
  an address-space section (bank 0 / const ROM / far RAM).

### Changed
- feat(lib): `sramLoad` / `sramLoadOffset` and `dsp1Raster` take `FAR`
  pointers (their ASM already honoured the bank); bank-0 callers are
  unchanged. The small hot-state helpers (`rect*`, `collide*`, `anim*`,
  `audioGet*`) stay bank-0-only by design.
- feat(examples): `games/breakout`'s 4708-byte tilemap/palette/brick
  buffers and `mode7/dsp1_ground`'s double-buffered M7 tables move to
  bank $7E — 1436 → 6880 and 1744 → 3937 bytes free in the C RAM band,
  both fbhash-identical.
- feat(runtime): the data-init record is `{addr16, bank8, size16, bytes}`
  and `CopyInitData` writes through a 24-bit pointer; crt0 zero-fills
  `$7E:2000-$FFFF` before `InitHardware`.
- feat(compiler): a mutable `const T *p` global is no longer sectioned
  into ROM (cproc's `.rodata` decision consulted the pointee's qualifier);
  the narrow `mul`/`shl` paths now fire on `u8`/`u16` indices too.
- docs(docs): `KNOWN_LIMITATIONS.md`'s "all C RAM below $2000" entry goes
  🔴 → 🟡: plain objects and the link-time ratchet are unchanged, `FAR` is
  the escape, the one remaining silent path is an explicit cast.

### Fixed
- fix(lib): **HDMA enabled mid-frame ran on stale A2A/NTRL** — a channel
  enabled during active display started at the next HBlank reading a
  "table" at `$00:0000` (the `tcc__r*` scratch) and wrote that residue to
  its destination register; `hdmaSetup*` now preset A2A = A1T and
  NTRL = 1 so the real first entry loads at the next HBlank. The
  `hdma/gradient_colors` and `hdma/hdma_helpers` baselines carried the
  glitch (CGRAM entry 2 zeroed, BG1HOFS stuck at 2). The HBlank reload
  rule this relies on is to verify against the SNES corpus.

## [0.38.0] — 2026-09-05

The DSP-1 Raster release: the coprocessor now streams the per-scanline
Mode 7 matrices of the Super Mario Kart / Pilotwings ground, and a new
example drives an F-Zero split with a real camera computed on the chip.

### Added
- feat(lib): **`dsp1Raster(ab, cd, vs, count)`** — the DSP-1 Raster
  command as a lib call: streams one A/B/C/D Mode 7 matrix per raster for
  the camera set by `dsp1Parameter`, straight into two
  `HDMA_MODE_2REG_2X` payloads (M7A/M7B, M7C/M7D), and closes the stream
  the way the official manual specifies (`$8000` written in place of a D
  read). Per-word RQM handshake with 16-bit port reads: 77 scanlines per
  100 rasters. **`dsp1Target(h, v)`** — screen point → ground plane, the
  inverse of Project for picking/aiming.
- feat(examples): **`mode7/dsp1_ground`** (rung 7.4) — the Super Mario
  Kart floor: F-Zero sky/ground split, a 126-raster floor streamed by the
  DSP-1 every frame into double-buffered HDMA tables, D-pad turns the
  camera (the floor rotates in true perspective, the sky pans) and drives
  forward with `dsp1Triangle` as the movement sin/cos. Firmware-gated luna
  manifest asserts the streamed raster count.
- feat(lib): `HDMA_DEST_BGMODE` and `HDMA_DEST_TM` destination constants
  for mid-frame mode/layer switches.
- docs(docs): DSP-1 tutorial gains "The ground" (Raster pipeline, raster
  coordinate convention, the imaginary-centre pivot rule, cost) and a
  stream-resync gotcha; Mode 7 tutorial gains the DSP-1 worked pattern
  and the CPU-side write rule under the shared M7 latch.

### Changed
- docs(lib): `dsp1Parameter`'s four outputs are documented as **Vof, Vva,
  Cx, Cy** (official manual §5.4.1, verified on luna) — the previous
  "Cx, Cy, two unconfirmed words" reading was wrong. Raster numbers are
  centre-relative and down-positive; the horizon is on screen line
  `112 + Vof + Vva`, the first finite ground raster is `Vva + 2`.
- docs(tech): `dsp1_reference.md` §15 records the Raster protocol as
  measured (DSP-1B LLE, luna v1.17.0): `$0A` + Vs, sentinel only on a D
  slot (SMK's "5 setup words" are Vs + 4×`$8000`), `$1A` never returns to
  command mode and `$80` Sync cannot recover it (unconfirmed on silicon),
  `Vs = 0` duplicates the first group; Parameter sweep and Target
  semantics.
- test: WRAM oracle re-baselined for `chips/dsp1_cube` (the `.dsp1_asm`
  section grew, moving stack return addresses — visual and KAT identical)
  and captured for the new example; corpus 84 → 85 across the anchored
  docs.

## [0.37.0] — 2026-09-02

The DSP-1 v2 release: the coprocessor module gains the canonical 3D
pipeline — true hardware perspective — plus the game-logic commands, and
the `dsp1_cube` example renders with a real perspective divide computed
on the NEC µPD77C25.

### Added
- feat(lib): **DSP-1 pipeline completed** — `dsp1Project` (world → screen
  H/V + depth scale M), `dsp1Parameter` (projection-plane setup, 7 in /
  4 out), `dsp1Distance` (hardware sqrt of a 3D vector), `dsp1Range`
  (sphere test for collision/LOD), `dsp1Present` (bounded known-answer
  probe that never hangs on a missing chip), and `fixed`↔T/A bridge
  macros between the SDK's 8.8 fixed-point and the DSP-1's 1.15/angle
  formats. Projection semantics characterised empirically on luna
  (DSP-1B) and recorded: the all-zero Parameter setup is degenerate,
  `azs=$4000` makes +Y the view axis, effective focal length ≈ lfe+les.
- docs(docs): new **DSP-1 tutorial** (`docs/tutorials/dsp1.md`) — chip
  model, RQM handshake, slot types, the Attitude→Objective→Project
  pipeline, Distance/Range for game logic, firmware gotchas; DSP-1 rows
  in `API_INDEX.md`; `snes.h` opt-in list now mentions `dsp1.h` and
  `panel.h`.
- ci: the native luna **test-manifests pillar now runs in CI** (it
  previously ran only locally via `make tests`); the DSP-1 manifest
  additionally asserts the full DR/SR handshake path (`dsp1_ok = 1`),
  firmware-gated so it SKIPs cleanly on firmware-less runners.

### Changed
- feat(examples): `chips/dsp1_cube` upgraded from orthographic to
  **hardware perspective projection**; `chips/README.md` gains the 11.5
  rung and now presents the three-chip family.
- build: `USE_DSP1 := 1` auto-adds the `dsp1` module to `LIB_MODULES`
  (aligned with the SuperFX/SRAM pattern).

### Fixed
- docs(examples): the dsp1_cube README documented a `luna run --dsp1-rom`
  flag that does not exist in the pinned luna — corrected to the
  `luna state --dsp1-rom` install path; the stale Python-probes section
  of the luna-test README replaced with the manifests view; dsp1_cube
  baseline provenance repaired (visual and WRAM baselines were captured
  from different builds).

## [0.36.2] — 2026-09-02

A patch release with one linker-visible compiler fix and a full audit of
the hardware documentation against authoritative SNES references
(fullsnes, Anomie, snesdev-wiki, the official Nintendo dev manual). The
audit corrected ten documentation findings; **no library or runtime
behaviour changed** (ROMs are byte-identical — WRAM oracle 84/84).

### Fixed
- fix(compiler,devtools): **anonymous C string labels are now TU-unique.**
  Two C files each using string literals could emit the same anonymous
  label (`string.15`) and wlalink rejected the link ("Label defined more
  than once") — cproc's per-process counter restarts each translation
  unit and the w65816 backend ships the labels as plain globals. The
  cc65816 wrapper now exports a per-file stem and cproc (submodule bump)
  prefixes the labels per TU. ROM output byte-identical; new
  compiler-test case guards the prefix.
- docs(docs,runtime,tech): **hardware claims disproven by the source
  audit corrected** across `KNOWN_LIMITATIONS.md`, `docs/hardware/` and
  eight tutorials. Highlights: the SA-1 SIWP/CIWP "disputed polarity" is
  resolved (bit=1 = write-enable; the Super Famicom Dev Wiki page is
  wrong — fullsnes, the official Nintendo dev manual §4.1.25 and nocash
  agree, so crt0's `$FF` was correct all along); the OAM pair-write
  latch applies to the low table only (high-table writes are immediate);
  hiding sprites needs Y=240 *plus* the X high bit for ≥32 px sprites
  (the lib already did this); the real NTSC VBlank budget is ~50,500
  master cycles (not 35,000); a scanline is 1364 master cycles (not
  "1369 at 3.58 MHz"); the HDMA repeat-mode doctrine is rewritten (PPU
  registers latch — the mode follows the data shape, verified on luna
  with an A/B/C probe); scroll writes take effect from the next scanline
  (not "latched at the start of each frame"); `sramSave` costs 7 CPU
  cycles/byte via MVN (an 8 KB save is ~1 frame, not "~3 %").

### Added
- docs(docs): **new documented trap** — BG1 scroll and the Mode 7
  matrix registers share one write-twice latch: an HDMA/IRQ write to a
  BG1 scroll register between the two writes of an M7 register silently
  corrupts the value and the MPY result (snesdev-wiki Errata). The lib
  is unaffected (it never reads MPY); gotchas added to the mode7 and
  hdma tutorials.
- docs(rules): new auto-loaded rule `hardware_claims.md` — new hardware
  claims in the docs must be verified against the arbitrated SNES
  corpus (Cartouche MCP) before landing; unverifiable claims are
  written as hypotheses.

### Changed
- refactor(lib): `nmiSet` is now a one-line forward to `nmiSetBank`
  (the v0.36.1 inline write dodged a QBE forwarding bug that turned out
  not to exist). Behaviour identical; WRAM oracle rebaselined for the
  jsl-instead-of-inline-stores shape.

## [0.36.1] — 2026-08-16

Two silent-failure fixes surfaced while building the `rpg` streaming overworld,
both affecting any project — not just the RPG.

### Fixed
- fix(runtime,lib): **dynamic-sprite RAM is now opt-in** — `crt0.asm` reserved
  the 768-byte dynamic-sprite VRAM upload queue (plus ~26 bytes of engine
  state) in **every** ROM, even those that never link `sprite_dynamic`. That
  ~794 bytes came straight off the 65816 stack budget (the stack grows down
  from `$1FFF` into the C-data region), so a game near the 8 KB WRAM ceiling
  could have its globals silently clobbered by a deep call chain — no error,
  just corruption. `oambuffer` had already been migrated to `sprite_dynamic.asm`
  to be opt-in; this finishes the job for the queue + state. Non-dynamic-sprite
  games reclaim ~794 bytes of WRAM (measured on the `rpg` overworld: stack
  headroom 410 → 1206 bytes). Transparent to dynamic-sprite users; validated
  pixel-identical across the corpus.
- fix(lib): **`nmiSet` installs the callback's real bank.** It hardcoded bank
  `$00`, so a VBlank callback the linker placed in bank `$01+` was installed at
  the wrong address and the NMI jumped into garbage. Post-A6 a function pointer
  is a 4-byte far pointer carrying its bank, so `nmiSet` now derives it. This
  also sidesteps a QBE miscompile that duplicated the pointer's high word when
  forwarding a 4-byte-by-value pointer parameter (`nmiSet` now writes
  `nmi_callback` inline). Any-bank VBlank callbacks work via plain `nmiSet`;
  no `nmiSetBank` or hand-rolled trampoline needed.

## [0.36.0] — 2026-08-14

A testing-infrastructure and sprite-tooling release, with one user-facing
compiler fix. The **luna-first migration completes**: every functional probe
now runs as a native `luna test` manifest — the last 19 Python probes retired,
zero left — and a stress campaign hardened luna across CPU/PPU/APU accuracy and
reliability (9 issues filed and shipped upstream, luna v1.14.0 → v1.17.0). Two
new asset tools — **palplan** and **aseprite2snes** — plus a full
Aseprite → animated-metasprite example round out the sprite pipeline.

### Added
- feat(tools): **aseprite2snes** — converts an Aseprite `--data --list-tags`
  export into OpenSNES `anim.h` `AnimClip` tables, one clip per tag, with
  per-frame durations (ms → ticks) and playback direction folded into the frame
  order. Pairs with `gfx4snes -P`: gfx4snes owns the pixels and metasprite
  table, aseprite2snes owns the timeline that indexes it. Golden-tested,
  documented (`docs/tools/aseprite2snes.md`).
- feat(examples): **aseprite_pipeline** — a sprites example whose every byte of
  animation data is machine-generated from one Aseprite project
  (`gfx4snes -P` + `aseprite2snes`), animated via `animTickMeta`/`oamDrawMeta`;
  press A to toggle the two generated clips. The living reference for the
  two-tool sprite pipeline.
- feat(tools): **palplan** — a project shared-palette planner that packs many
  `.pal` files into the SNES's 8 BG + 8 sprite CGRAM slots, merges identical
  palettes, fails loudly on over-subscription, and emits a C header of named
  offsets. Wired into the `rpg` example's sprite-palette layout.

### Fixed
- fix(compiler): a **conditional or ternary that selects a far pointer no longer
  drops the pointer's bank byte** — a silent-failure codegen bug. qbe's phi
  lowering stored only the low 16 bits of a `Kl` (4-byte far) phi result, so
  `x ? "A" : "B"` or `cond ? pa : pb` passed a corrupt pointer (wrong bank) to a
  call, return, or store. Now both halves are emitted. Guarded by a compile-time
  pin and the `a6_farptr` runtime matrix's `ph2` cell.

### Changed
- test(luna-test): **the luna-first harness migration is complete** — all 19
  remaining Python functional probes were ported to native `luna test`
  manifests (input/mouse/Super Scope, SRAM round-trip, DSP registers, DMA
  budget/safety, OAM decode, audio energy, coprocessor execution, …). The
  functional harness is now `luna test` over ~47 manifests; `probes/` retains
  only the thin orchestration helpers.
- test(luna-test): a **luna stress campaign** added accuracy and reliability
  regression probes — hardware math (`$4202`-`$4216`), open-bus/MDR, 65816
  decimal mode (BCD), PPU sprite-per-line overflow (STAT77 range/time-over),
  and a full 94-tool MCP surface sweep — each cross-checked luna == Mesen2 ==
  published reference. Nine luna issues were filed and shipped (luna v1.14.0
  → v1.17.0); the pinned binary tracks v1.17.0.
- test(compiler): the `a6_farptr` far-pointer runtime matrix is **fully green**
  (14/14) — the A6 far-deref gap closed and its stale `KNOWN_FAIL` cells were
  cleared, so every deref form is now an active regression guard.

### Documentation
- docs: the **OpenSNES showcase is the doc-site landing page**, and the craft
  and tools sections are woven into the newcomer journey.

## [0.35.0] — 2026-08-07

A documentation-and-tooling release. The developer-experience push from 0.34.0
continues: a static, build-time asset-budget report to pair with the runtime
one, a complete **Tools** documentation section so newcomers discover and learn
every asset converter from the docs, and two more game-craft guides.

### Added
- feat(devtools,build): **static VRAM/CGRAM asset-budget report** — a new
  `make asset-budget` command that weighs the converted graphics on disk (tiles
  and maps → VRAM, palettes → CGRAM) against the 64 KB / 256-colour limits, the
  build-time twin of the runtime `make budget`. It also prints a compact
  `[ASSETS]` instrument line at every example link (silent for asset-less
  builds, `SKIP_ASSET_BUDGET=1` to disable). Report-only, never a gate — an
  inventory upper bound, since streaming and palette swaps legitimately exceed
  the limits.

### Documentation
- docs(tools): a **complete Tools documentation section** (`docs/tools/`) — a
  toolbox landing page plus a presentation-and-tutorial page for each converter
  (gfx4snes, tmx2snes, img2snes, font2snes, wav2brr, smconv), wired into the
  doc site and nav. Facts verified against each tool's README and live `--help`.
- docs(craft): two **craft companions** — *camera* (deadzone, look-ahead,
  platform snapping, room-locking, all derived from one camera value) and
  *game-feel* (screen shake, hit flash via color math, hitstop, fades, palette
  cycling — the SNES-specific tricks).
- docs(craft): a **cost-by-mode table** in the planning guide — per-mode layer
  layout, bytes/tile by colour depth, and the VRAM trade for each background
  mode.
- docs(rules): the **luna-first** transitory-tooling convention (internal
  contributor rule) — everything validates through luna; internal scripts are
  transitory prototypes to be retired once luna owns the capability.

## [0.34.0] — 2026-08-04

The DSP-1 & developer-ecosystem release. OpenSNES gains first-class **DSP-1**
(NEC µPD77C25) support — the third user-programmable SNES coprocessor beside
SA-1 and Super FX — with a pseudo-3D cube example, alongside a wave of
developer-experience tooling: a project starter, a WAV→BRR sound tool, a PPU
budget report, and a hardware-grounded game-craft documentation track. The
example corpus grows 80 → 83.

### Added
- feat(lib,examples): **DSP-1 coprocessor support** (NEC µPD77C25) driven from
  C — `snes/dsp1.h` command wrappers (Multiply, Triangle sin/cos, Rotate,
  Attitude + Objective for 3D transforms, and `dsp1Init` for the `$80` resync
  handshake), the `USE_DSP1` build mode (cartridge-type `$03`), and a
  `chips/dsp1_cube` pseudo-3D example — 8 cube corners rotated in 3D by the
  DSP-1 each frame. Each command was verified on luna against known math.
  Firmware-gated tests: luna LLE-emulates the DSP-1 and needs Sony's
  (non-shippable) `dsp1b.rom`, so CI skips the firmware-dependent pillars
  cleanly while a dev with the dump gets full coverage
- feat(tools): **wav2brr** — a WAV→SNES-BRR sample converter that reuses
  smconv's own BRR encoder, plus a zero-config `.wav` → `.brr` build rule and a
  `audio/sfx_from_wav` one-shot-SFX example
- feat(build): **opensnes-starter** — a copy-out / "Use this template" project
  (a movable-sprite game with a CI workflow) that consumes the SDK via
  `OPENSNES=`, shipped in the release zip
- feat(luna-test): **`make budget`** — a per-example PPU VRAM/CGRAM/OAM
  footprint report, the runtime companion to the planning guide
- feat(examples): **backgrounds/mode2** — offset-per-tile (OPT) per-column scroll
- docs(craft): a **game-craft guide track** (`docs/craft/`) — planning your
  game, composing backgrounds, the frame budget, and from-tiles-to-levels:
  hardware-grounded *design* advice that links the universal and owns the
  SNES-specific

### Changed
- test(luna-test): luna pin **v1.9.0 → v1.13.0** — adopts the DSP-1 visibility
  work luna shipped from our request (`--dsp1-trace` / `--dsp1-trace-commands`
  and `dsp1.instructions_executed`); adds a DSP-1 coproc-liveness probe assert;
  makes `install-luna.sh` curl-first (fixes a 401 on the now-public luna repo,
  local and CI); the visual/WRAM corpus is re-baselined (benign — the ROMs are
  byte-identical, only luna's rendering/timing moved across the version bump)

## [0.33.0] — 2026-08-01

The example-curriculum release. The corpus is reorganized from the old
`graphics/{backgrounds,sprites,effects}` grouping into 17 use-case
families, grows from 74 to 80 with new showcase and capstone examples that
fill the gaps a learner would actually hit, and a latent CPU-hang in
`textPrintU16` is root-caused and fixed along the way.

### Added
- feat(examples): eight new examples across the reorganized curriculum —
  **palette_cycle** (Colour 6b) and **shadow_tint** (6c.2) complete the
  Colour family; **sprite_swarm** caps the sprites family with an honest
  OAM-throughput showcase (and the measured ~32-sprite 60 fps ceiling for
  per-sprite C motion); **panel_hud** demonstrates the 9-slice `panel`
  module in isolation; **game_skeleton** is the smallest complete
  title → play → game-over game, a fork-ready capstone; plus
  **move_sprite** (Input 4.2), **scroll_message** (Text 1.4) and **echo**
  (Audio 8.9, S-DSP reverb) from the reorg waves
- docs(examples): every family now has a README "ladder" that walks a
  learner rung by rung, and a pedagogy charter + reframed LEARNING_PATH
  present the whole corpus as a seven-stage developer's journey

### Changed
- docs(examples): the example corpus is reorganized into 17 use-case
  families (text, fundamentals, backgrounds, sprites, scrolling, mode7,
  hdma, color, windows, transitions, input, audio, maps, basics, memory,
  chips, games) — `graphics/` is dissolved entirely; a naming audit
  renames examples to consistent per-family patterns (the redundant
  `mode7_` prefix dropped, `object_size` → `sprite_sizes`, `mapscroll` →
  `map_scroll`, `slopemario` → `slope_collision`, `hires_text` →
  `mode5_hires`, `parallax_scrolling` → `parallax_scroll`)
- docs(examples): low-value duplicates discarded (`hicolor_hires`,
  `snesmod_music_hirom`); `text_test` renamed to `print_string`

### Fixed
- fix(lib): `textPrintU16` rewritten to a codegen-robust MSD-first loop.
  The old reversed-buffer/decrementing-pointer form miscompiled under
  some link layouts — the pointer underflowed the stack buffer and the
  CPU hung forever in `textPrint` — a latent hang for any game printing a
  number in a sprite-using loop. Output is byte-identical
  (`.claude/notes/tech/textprintu16_codegen_hang.md`)
- fix(examples): track `echo`'s `pp.brr` — the `*.brr` gitignore rule
  silently dropped it, breaking clean-checkout CI
- fix(luna-test,devtools): rename-sweep stragglers (movement-probe ROM
  path, ATTRIBUTION path, doc references)
- test(luna-test): re-key the WRAM oracle to the reorg'd example names and
  prune 44 stale pre-reorg keys — one baseline per example

## [0.32.0] — 2026-07-26

The follow-through release. The #127/#128 asset work grew its second
half, the documentation caught up with the code, and the toolchain moved
onto the upstream wla-dx v10.7 release.

### Added
- feat(build,examples): `ASSET_SECTION` picks the bank itself.
  `SEMISUPERFREE BANKS 7-1` walks from the top bank down, so declared
  payload never lands in bank $00 unless nothing else fits — no
  per-project bank choice, no silent code-bank pressure. Every link now
  reports how much declared payload ended up in bank $00 (#127)
- feat(tools): `tmx2snes -C` emits a per-cell collision grid — one byte
  per map cell from the Tiled collision layer — alongside the existing
  `-e`/`-Q` outputs, with a golden in `make test-tools` (#128)

### Changed
- build(submodule): wla-dx pin advanced to the upstream **v10.7**
  release. The SPAN regression that blocks a straight follow of `master`
  is not present in v10.7; it is root-caused, reduced to a minimal repro,
  and filed upstream as vhelin/wla-dx#729 (notes in `.claude/notes/tech`)
- docs: new `map` and `panel` tutorials (both modules shipped without
  one); `game_states` reframed to lead with the `scene` stack and keep
  the manual `switch` as "under the hood"; the example index lists the
  full 74 (two complete games were invisible); stale counts and the
  Mesen2→luna narrative corrected across the guides

### Fixed
- fix(docs): every example screenshot has a unique basename. All 74
  READMEs referenced `screenshot.png`; Doxygen flattens images by
  basename, so one capture (Mario, from `mapandobjects`) served every
  page — audio pages included. Audio pages now carry no screenshot (no
  visual to show) and three effect captures were regenerated to show
  their actual effect. `IMAGE_PATH` lists each example dir and a
  doc-drift guard fails the build on a duplicate basename, a missing
  file, or a dir absent from `IMAGE_PATH`
- fix(docs): tutorial claims that no longer matched the code — the
  removed "DMA source must live in bank $00" restriction, `oamInit()`
  written without its arguments (would not compile), dead `.claude/*.md`
  links, and Mesen2 described as the accuracy reference (luna is)

## [0.31.0] — 2026-07-22

Written by building a real game. The RPG template (#7) was the forcing
function: every silent failure it hit became a fix, a guard, or a
sentence someone else will not have to rediscover at 2am. 74 examples.

### Added
- feat(examples): RPG template driven by Tiled — `res/town.tmj` carries
  terrain, per-tile collision, entity positions *and* each villager's
  line of dialogue, so adding a villager is a map edit. Two scenes (the
  town and a house interior, each with its own tileset, palette,
  collision and entities), a HUD with hearts and a purse, tile-exact
  collision and a 9-slice dialog box (#7)
- feat(lib): opt-in `panel` module — 9-slice boxes on a BG layer.
  Several panels share one tilemap and one upload, the caller owns the
  buffer (2 KB is a quarter of a game's RAM band — the cost stays
  visible), and `panelFlush()` does its DMA under forced blank so the
  #130 trap cannot be reached through it (#123)
- feat(tools): `tmx2snes -e` emits the Entities layer as C defines
  (struct shape + rows, custom properties included) and `-Q` emits a
  quadrant-ordered 64x64 tilemap for the `background` module. The `-Q`
  golden is byte-identical to the RPG's hand-written Python converter.
  New golden suite in `make test-tools` (#128)
- feat(tools): `gfx4snes -c FILE` imposes a palette and fails, naming
  the colour and the pixel, when the image uses one that is not in it.
  Adaptive quantisation is a function of the whole image, so adding a
  tile re-derives all 16 slots and shifts every existing tile's hue —
  which is how the RPG's roads turned pink (#131)
- feat(build): `ASSET_SECTION` (templates/assets.inc, included in every
  assembled file) plus a post-link report of how much asset payload sits
  in bank $00. Assets travel as far pointers or const far reads, so they
  never needed the code bank; the RPG went from 12 to 9902 free bytes
  there (#127)
- docs: `docs/API_INDEX.md` — the SDK indexed by what you are trying to
  do, with the example that does it. Its example paths are checked by
  the doc-drift sentinel (#126)

### Fixed
- fix(compiler): a `const` array of structs indexed at runtime kept its
  bank byte. Three defects in a chain, the first two masking each other:
  `convert()` used QBE's native class model (`l` is 8 bytes) on a target
  where `l` is 4; the pointer-arithmetic widening keyed on the C type
  kind, which repaired that by accident while truncating already-wide
  member addresses; and QBE's load forwarding, reachable only once the
  first two were fixed, served byte loads out of a word load by shifting
  16 and 24 bits from a load that fetched 2. Closes #132 and takes the
  A6 far-pointer matrix from 7/8 with 4 XPASS to **8/8 with 5 XPASS**
- fix(lib): `collideTile`/`collideTileEx`/`collideRectTile` take a
  `const` tilemap, so a collision map may live in any bank. It compiled
  to `lda.l $0000,x` before — bank $00, silently, whatever the map's
  real bank. `collision_demo` had paid 224 bytes of WRAM to work around
  it
- fix(lib): pointers to caller-owned read-only data are `const`
  throughout `dma`, `background`, `sprite`, `map`, `lzss`, `object` and
  `sram`. The SDK was forcing `const` *off* users' own assets, which
  then made every C index of them a bank-$00 read (#122)
- fix(lib): `oamInit`'s second parameter is named `name_base` and
  documented as a page number 0-7, with `OBJ_NAME_BASE(addr)` to convert
  from a VRAM address. Passing an address — the natural mistake — masks
  to 3 bits and renders sprites as background garbage (#122)
- fix(tools): `tmx2snes` minifies the JSON before parsing, so a
  pretty-printed `.tmj` works; a real parse failure now reports
  cute_tiled's reason and line instead of a bare "Cannot load map"
  (#125)
- fix(tools): `gfx4snes` warns when an image is not an exact multiple of
  the block size — the last row is built from pixels that are not there
  (#122)

### Documentation
- docs(lib): the VRAM write window on `setBrightness` and `dmaCopyVram`.
  `setBrightness(0)` blacks the screen but leaves the PPU fetching, so
  writes are still dropped; only `setScreenOff()` opens the window. The
  failure survives testing — a small transfer lands in VBlank and is
  correct every time (#130)
- docs(lib): `dma.h` claimed source data must live in bank $00. That has
  been false since chantier A6; the stale note is where a wrong bank-$00
  claim about `dmaCopyCGram` in #122 came from
- docs(lib): `oamHide()` carries the reason to call it — OAM coordinates
  wrap, so an off-camera entity reappears somewhere plausible rather
  than vanishing (#129)
- docs: the top-down straddle convention in the collision tutorial, and
  the `-s` rule in the sprite tutorial (`-s` is the SPRITE's size, not
  the tile size) (#124, #122)

### Testing
- test: `make tests` now runs the per-frame WRAM oracle, which only CI
  ran. The gate contributors are told to run must be the gate CI runs

## [0.30.0] — 2026-07-15

The PeterLemon port series: six krom demos reproduced in C with measured
parity (pixel-exact / IoU 1.0 / cycle-exact cadence), each landing the
SDK surface it exposed as missing. 63 examples.

### Added
- feat(lib): H/V-timer IRQ surface — `irqSet`/`irqSetBank`/`irqClear`,
  `irqSetHTimer`/`irqSetVTimer`, `irqEnable`/`irqDisable`; raw crt0
  dispatcher (`jml [irq_callback]`, zero imposed overhead) with the
  ASM-only handler contract documented in `interrupt.h`
- feat(lib): SETINI video surface — `videoSetInterlace`,
  `videoSetObjInterlace`, `videoSetOverscan`, `videoSetPseudoHires`,
  composing through a write-only-register shadow
- feat(lib): `hdmaSetupIndirect` (the indirect HDMA the header promised)
  + `HDMA_DEST_M7B/C/D` defines
- feat(examples): six krom (Peter Lemon) ports, each with a
  register-fidelity table and measured parity in its README —
  `hdma_wave_table` (repoint animation, residual-0 displacement field),
  `hdma_indirect_gradient` (pixel-exact), `hicolor_1792` (1792 colors
  via H-IRQ CGRAM streaming, cadence cycle-exact in luna AND Mesen2),
  `mode7_perspective_rotate` (full matrix per scanline, IoU 1.0000 at
  90°), `hires_text` (512×448, Mode 5 + interlace), `window_multi_hdma`
  (both windows per scanline), `gradient_9bit` (0 differing pixels)
- feat(devtools): `check_bank_reads.py` (bank-blind C read post-link
  guard, #104), `check_corpus_fresh.py` (stale-corpus gate, #105),
  `m7ptables.py` (Mode 7 table extraction + 32256/32256 math
  verification), `hicolor64.py` (per-tile-row palette converter)
- test(luna-test): blank-baseline guard — a broken ROM can no longer
  self-certify a black frame (#115); NMI-context math + #114 + #99
  regression pins in the libtest ROM (24 vectors)

### Fixed
- fix(runtime): NMI-safe mul/div — C's `*`/`/`/`%` inside nmiSet
  callbacks silently returned auto-joypad garbage (hardware mul/div
  unit conflict); the runtime now takes software paths in NMI context
  (#113). `fixMul`/`fixLerp` stay hardware and carry loud warnings.
- fix(compiler): signed division through explicit casts — cproc's
  stale pre-16-bit-int signedness heuristic forced `(s16)` casts of
  unsigned-derived operands back to unsigned div/mod/shr/compares
  (#114); signedness now follows the operand type per the C standard
- fix(lib): `consoleInit` left the OPHCT/OPVCT latch read pointers
  mid-sequence (single-read rand seeding) — every later latched
  H/V-counter read was hi/lo-swapped garbage
- fix(lib): `nmiSetBank` restored NMITIMEN from a literal, clobbering
  timer-IRQ enable bits — all $4200 writes now compose through a shadow
- fix(lib): map getters read their $7E tables with long addressing —
  usable from C (#103)
- fix(examples): `fix32_orbit` renders its orbiting dot for the first
  time — mis-interleaved 4bpp tile data + wrong OBSEL base, masked for
  weeks by a black baseline (#115); `oamInit` tile_base doc corrected
  ($2000-word steps, not $1000)

### Changed
- docs: effects-arc techniques integrated into the hdma/window/mode7/
  graphics tutorials; hi-res physics explainer; KNOWN_LIMITATIONS entry
  for fixMul-in-NMI
- build: targeted gitignore artifact names replace the *.asm/*.bin/*.h
  catch-alls (fresh-checkout builds were losing committed assets)
- chore(luna-test): pinned luna bumped to v1.9.0 (`--force-mapper` on
  run, typed `dma.channels[]` state, `luna frames`)

OpenSNES is forked from [PVSnesLib](https://github.com/alekmaul/pvsneslib). This changelog
covers changes made since the fork.

## [0.29.1] — 2026-07-09

Two compiler/tool correctness fixes surfaced while wiring the anim module
(#97) — both matter for editor-generated content (Cooper).

### Fixed
- fix(compiler): u8 read-modify-write through a pointer now re-reads the
  correct page (#99). qbe w65816 ran the 16-bit pointer-address reload in a
  byte load's indirect path while still in 8-bit mode (a preceding byte
  store left A 8-bit and byte ops skip the entry `rep #$20`), so it loaded
  only the pointer's low byte and dereferenced the wrong page — for structs
  outside page zero. Reached `consoleInit`'s PPU-register read; the fix adds
  the `rep #$20` the store-indirect path already used. Pinned by
  `test_rmw_ptr_reread` (codegen) and the libtest `r_rmw_u8` runtime vector.
- fix(tools): gfx4snes `-T` emits 8x8 OAM character names, not block indices
  (#100). `metasprite_save` emitted the map's block index while the saved
  `.pic` is a fixed 128px-wide raster, so the tables rendered wrong for
  16/32px blocks. Now emits positional names (`(p/bpr)*(sub*16)+(p%bpr)*sub`);
  the `-F` path stores the canonical block's reading-order position so
  flip+canonical stays correct for multi-mirror sheets. Two golden fixtures
  added.

### Changed
- chore(luna-test): rebaselined 2 visual + 49 WRAM streams. The #99 fix adds
  4 instructions to `consoleInit` (called by every example); captures keyed
  on instruction-step count shift by that offset — same behaviour, offset
  state (verified: 54/56 render identically, the 2 timing-sensitive displays
  render valid frames, `is_pal_system` unchanged).

## [0.29.0] — 2026-07-07

Cooper's two asks land: a **declarative animation player** (the anim module,
with a public layout contract editors can emit) and **`make test` for user
game projects** (the SDK's luna harness, packaged and project-rooted).
Dogfooding the new module surfaced two compiler bugs and a hole in the
bank $00 ratchet — all fixed at the root layer, and the whole class of
platform-dependent lib const spills is now structurally impossible.

### Added
- feat(lib): `anim` — opt-in declarative animation module (#97). AnimClip
  (ROM, 12 bytes) + AnimPlayer (RAM, 8 bytes) are a public tooling contract;
  `animPlay` (continue-if-same) / `animTick` / `animTickOam` /
  `animTickMeta` / `DECLARE_ANIM_CLIP`; frame values are opaque u16
- feat(tools,build): project test harness — `make test` / `make test-update`
  in any user project, opt-in by the presence of `test/manifest.toml` (#98).
  Three oracles: WRAM asserts by symbol name, fbhash visual baselines
  (multi-point), and the in-ROM SNES_ASSERT/WDM channel. `opensnes test`
  CLI command; the game template scaffolds a verified manifest
- feat(tools): gfx4snes `-T` emits OBJ_FLIPX/OBJ_FLIPY attributes when tile
  dedup kept a flipped variant (golden-tested; closes the metasprites.c TODO)
- ci: end-to-end user-story step (init → build → test-update → test,
  including the failure half)
- docs: animation tutorial leads with the anim module; GETTING_STARTED gains
  "Test your game"; MetaspriteItem tile units documented as a contract
  (gfx4snes -T block-unit mismatch tracked as #100)

### Changed
- refactor(examples): animated_sprite, metasprite and likemario animate via
  the anim module (metasprite is now self-animating with two-point capture;
  likemario's mario_animate() is a 6-line state→clip mapping)

### Fixed
- fix(compiler): ROM-const numeric DW/DL fields emit the full 4-byte slot —
  WLA's `.dl` truncates numbers to 24 bits (found by dogfooding; the u8 RMW
  pointer bug also found is tracked as #99 with an XFAIL vector)
- fix(devtools): symmap hard-fails ANY C const section spilled past bank $00
  by parsing the linker's `.rodata.N` sections — named top-level statics
  were invisible to the old string.N heuristics (likemario's clips shipped
  a silently dead animation right through the ratchet)
- fix(lib,devtools): lib C modules carry no ROM const data (hdma sine +
  channel masks, text hex digits, math atan LUT) — WLA-DX section placement
  varies by platform, so a tight example could spill a lib LUT to bank $01+
  on one OS only (caught live on the macOS CI leg). Enforced by a new
  `make lint` check (`devtools/check_lib_rodata.py`)
- build(release): the SDK zip now ships the project test harness AND the
  post-link checks (`symmap.py`, `check_nmi_wram_race.py`) that
  make/common.mk runs — release users were silently skipping both
- fix(tools): `opensnes init <absolute/path>` no longer breaks the generated
  Makefile (leaf name used for TARGET/ROM_NAME)

## [0.28.0] — 2026-07-06

luna becomes OpenSNES's **sole emulator** — automated pillars *and* a
feature-complete interactive debugger. A same-week collaboration with the
luna project (epic luna#63) shipped first-class breakpoints, native WLA-DX
symbol resolution and a 39-tool MCP surface; OpenSNES consumed each release
the day it landed. Mesen2 is now an optional third-party alternative, the
Lua debugging layer is gone entirely, and the test harness asserts by
variable name.

### Added
- docs(tutorials): `debugging.md` — the six standard interactive-debugging
  workflows on luna's CLI + MCP surface (state inspection,
  who-writes-this-variable via breakpoints, break-at-function, OAM
  shadow-vs-hardware, poke-and-observe, temporal-artefact capture)
- feat(tools): `opensnes doctor` detects the luna CLI (pinned binary,
  `$LUNA_BIN`, or PATH) and reports its version; `luna-gui` joins the
  GUI-emulator detection for `opensnes run`

### Changed
- chore(luna-test): pinned luna bumped v1.1.0 → v1.7.0 (v1.6.0
  interactive-debugger parity + v1.7.0 GUI step/breakpoints and CLI symbol
  resolution). One legitimate visual/WRAM rebaseline pair at v1.6.0
  (HDMA-faithful gradient, timing-phase cube rotation), zero drift at v1.7.0
- refactor(luna-test,test): the probes and runtime-assertion ROMs assert and
  peek **by symbol name** — luna resolves the auto-detected `<rom>.sym`
  natively, and the harness's duplicated wlalink parser is deleted
- docs(rules): luna-first debugging everywhere — the workflow rules that
  still made a manual Mesen2 pass mandatory (in contradiction with
  testing.md since the luna migration) are re-pointed; Mesen2 remains cited
  only as an optional alternative and as the historical accuracy reference

### Removed
- refactor(devtools): `devtools/snesdbg/` — the Mesen2-bound Lua debugging
  library (~2,360 LOC, zero automated usage). Its four workflows live on in
  `docs/tutorials/debugging.md` over luna MCP; no Lua remains in the
  repository

### Fixed
- fix(luna-test): the aim_target WRAM baseline had been captured from a
  stale local ROM — caught by the WRAM CI gate disagreeing on both arches,
  restored from a clean build (the example had in fact never drifted)

## [0.27.1] — 2026-07-05

Documentation-site branding and repository housekeeping.

### Added
- feat(docs): OpenSNES brand theme for the documentation site — palette
  extracted from the logo (SNES-violet primary in light mode; logo-navy page
  with terminal-green links in dark mode), the logo's tri-color accent bar as
  a page-top signature, and the logo + wordmark now present on every page.
  Every text/background pair verified WCAG AA

### Fixed
- fix(docs): the docs site rendered a raw "Toggle main menu visibility"
  checkbox on every page and showed no project name or logo anywhere — the
  custom Doxygen header had dropped the standard title area and the
  `tabs.css` link; both restored

### Changed
- docs: stale migration-era claims retired (luna-test "Phase 1 prototype"
  framing, dead `/tmp` report links, BENCHMARK.md "currently DISABLED" while
  the re-homed suite gates CI, contradictory fbhash/SHA-256 phrasing)
- build: `git status` no longer stays noisy from the pinned wla-dx
  submodule's in-tree build artifacts (`ignore = dirty`; SHA drift remains
  guarded by `make verify-toolchain`)

## [0.27.0] — 2026-07-04

Hardening release, driven by a full project review: silent-failure fixes in
the library, a broad test-coverage wave (WRAM CI gate, linux/arm64 functional
leg, asset-tool golden tests, lib assertion ROM, compiler-fixture ratchet,
multi-point visual regression), a root-cause verdict on the historical MSYS2
cproc segfaults, and a first API pruning.

### Added
- test(lib): `libtest` assertion ROM — executes lib functions with known
  vectors (div16/mod16/mul16/sqrt16, text cursor-wrap sentinel) and asserts
  the WRAM results via luna; wired into `make tests` and CI
- test(tools): golden-output tests for gfx4snes and smconv (`make test-tools`
  + a lint-workflow job) — the asset converters previously had zero tests
- test(luna-test): the WRAM-state regression now gates CI on both arches
  (54/56 examples; the two cross-arch-fragile ones are excluded by default,
  `--all` includes them against a same-arch baseline)
- test(luna-test): multi-point visual regression, opt-in per example via
  `manifest.toml` `steps = [a, b]` — a timing shift breaks some-but-not-all
  points (reported as "timing drift?") while a real regression breaks all;
  enabled for the two self-animating examples
- ci: the functional-tests job runs on linux/arm64 too (a shipped release
  artifact never executed a ROM in CI before), and release builds get a full
  luna corpus-liveness smoke pass on both Linux arches before upload
- ci(compiler): cproc segfault-retry telemetry — every Windows build reports
  its retry count in the job summary; the MSYS2 diagnostic workflow is
  repaired (it referenced a dead test tree) and now a monthly gating monitor
- feat(devtools): doc-drift sentinel extensions — soft-wrapped (multi-line)
  count claims, `Examples (N)` / `(N / N)` forms, example-path existence,
  per-category sums in `examples/README.md`, ROADMAP footer date, and a
  stale-ASM-bank-comment anchor
- build(tools): `make clean-examples` (keep the toolchain, clean the ROMs);
  `opensnes doctor` checks the host `cc`; `make verify-toolchain` prints the
  (unpinned) host preprocessor identity
- build: `USE_SUPERFX := 1` auto-adds the `superfx` lib module, like
  sram/snesmod

### Changed
- refactor(lib): **`oamSetVisible` and `OBJ_SHOW`/`OBJ_HIDE` removed** — the
  show direction was structurally a silent no-op (SNES visibility is
  Y-position-based, there is nothing to restore); use `oamHide()` to hide and
  `oamSetY()`/`oamSet()` with a valid Y to show
- test(compiler): all 66 fixtures compile on every run (56 were previously
  never exercised) with a ratchet on unchecked cases; `test_metasprite`
  compiled for the first time (missing SDK include path, caught immediately)
- test(compiler): the `a6_farptr` far-deref runtime gate is wired into
  `make tests` and CI (xfail-tolerant; runtime fixture ROMs now rebuild from
  clean so a stale artifact can't produce a misleading verdict)
- ci: the Windows make-level retry loop is dismantled — a broken build fails
  honestly; the narrow cc65816-level retry (exit-139-only) stays, counted
- ci: pinned Doxygen is fetched from the official GitHub release with SHA-256
  verification (doxygen.nl started returning 403 to CI runners; the download
  was also the last unverified binary fetch)
- docs: ROADMAP, `examples/README.md` and `GETTING_STARTED.md` resynced with
  the real tree (56 examples, regenerated inventory, shipped CLI status,
  dead `superfx_3d` path fixed)

### Fixed
- fix(lib): **text cursor overflow** — `cursor_y` was never bounded, so
  printing past row 31 wrote beyond `tilemapBuffer` into adjacent bank-`$00`
  WRAM (silent corruption); `textPutChar` wraps, `textFillRect`/`textSetPos`
  clamp
- fix(lib): `RGB()` masks each component to 5 bits so an out-of-range value
  can't bleed into the neighbouring channel (compile-time-folded at every
  existing call site — ROMs verified bit-identical)
- fix(luna-test): the WRAM runner could hash a stale trace file when
  `wram-trace` failed silently, poisoning the baseline (caught live); it now
  unlinks the output first and checks the exit code
- fix(build): the cc65816 wrapper's cproc stderr capture moves from a
  hardcoded `/tmp` path to `mktemp` under the cleanup trap
- docs: stale pre-A6 "BANK LIMITATION" claims in `dma.asm`, a contradictory
  bank-`$00` threshold comment in `common.mk`, the phantom `sed` stage in the
  pipeline docs, and pre-luna-migration claims in the test docs reconciled

### Performance
- perf(lib): `div16`/`mod16` rewritten as bounded binary long division —
  the previous repeated-subtraction loop was O(quotient)
  (`div16(65535, 1)` took 65535 iterations)

## [0.26.0] — 2026-07-02

Source-level C debugging for the Cooper VS Code extension, plus the animated
example gallery and a docs-integrity pass.

### Added
- feat(compiler): source-level debug info, opt-in via `CC65816_G` — `; @cline`
  PC↔C-line markers, typed `; @dbglocal` locals, and a `.dbg` aggregate-layout
  sidecar, consumed by Cooper. A normal build stays byte-identical (verified in
  CI); debug metadata is emitted only under `-g` (#92)
- feat(examples): animated `demo.gif` for 7 dynamic examples (mode7,
  parallax_scrolling, transparency, hdma_wave, hdma_helpers, mosaic, fading) (#90)
- ci(compiler): CI coverage for the `-g` debug path (`check_debug_info.sh`) (#92)

### Fixed
- fix(compiler): debug metadata no longer perturbs release codegen — gate the
  emission behind `CC65816_G` (cproc) and exclude `dbgloc` from inline
  eligibility (qbe), so inline helpers like `colorMathEnable` don't spill a
  duplicate symbol (#92)
- docs: repoint the dangling references to the removed `hdma_gradient` example (#91)

## [0.25.0] — 2026-06-29

Onboarding & developer-experience release. The first impression now matches the
project's goal of being approachable at any level, and the `opensnes` project CLI
— previously uninstalled and undocumented — is shipped as the headline "start
your own game" path.

### Added
- feat(build,tools): ship the `opensnes` project CLI (`init` / `build` / `run` /
  `doctor`) — installed into `bin/` and the release zip; scaffolds a working
  project from `blank` or `game` templates (#88)
- docs: `SECURITY.md` — private vulnerability disclosure policy and supported
  versions (#87)
- docs: `examples/basics/fix32_orbit` gains a README + screenshot, completing the
  "every example has a README and screenshot" guarantee (#87)

### Changed
- docs: README "Who is OpenSNES for?" reframed from a gatekeeping list into
  on-ramps by level (new-to-SNES / comfortable / porting+perf) (#87)
- docs: `GETTING_STARTED.md` "Create Your Own Project" is now CLI-first, with the
  hand-written Makefile + `main.c` kept as a manual alternative (#88)

### Fixed
- fix(tools): the CLI `blank`/`game` templates now compile against the current
  API (`textModeInit()`); the `game` template renders a visible, D-pad-movable
  sprite (#88)
- fix(build): `.vscode/tasks.json` test tasks pointed at `tests/*.sh` removed in
  the luna migration — now `make tests` / `make test-compiler` (#87)
- docs: link `luna` mentions in the README to the luna project

## [0.24.0] — 2026-06-28

First release with a native Linux **aarch64 (arm64)** SDK package. The
release pipeline now builds on a GitHub-hosted `ubuntu-24.04-arm` runner
alongside the existing x86_64 Linux, macOS, and Windows targets.

### Added
- ci(release): add Linux arm64 (aarch64) build target — each tagged
  release now ships `opensnes_<tag>_linux_arm64.zip` (#85)

## [0.23.0] — 2026-06-23

API-consistency & documentation-quality release, from a 3-agent audit (API
currency, magic numbers, developer experience). One breaking API fix, two new
constant sets, and a deep doc-rendering fix.

### Changed
- **BREAKING — `textInit()` takes a VRAM word address** (was a byte address). It
  was the SDK's lone unit inconsistency: every other call (`dmaCopyVram`,
  `bgSetGfxPtr`/`bgSetMapPtr`, `oam*`, `textLoadFont`) already uses word
  addresses. Migration: pass `W` where you used to pass `W * 2`;
  `TEXT_DEFAULT_TILEMAP_ADDR` is now `0x3800`. All in-repo call sites updated;
  runtime result unchanged (visual regression 56/56).

### Added
- `VMAIN_INC1` / `VMAIN_INC32` (registers.h) — named VRAM-increment modes,
  replacing raw `REG_VMAIN = 0x80/0x81`.
- `SCREEN_WIDTH` (256) / `SCREEN_HEIGHT` (224) (video.h).

### Fixed
- **Docs rendering**: inline `code` spans inside `"..."` quotes were mangled by
  Doxygen (literal `<tt>`/`<em>` tags / raw backticks leaking into the published
  pages). Fixed the affected tutorial + header spans, and **pinned Doxygen to
  1.16.1** for the docs build (ubuntu's 1.9.8 has a severe form of the bug that
  was breaking the live GitHub Pages site).
- **Onboarding doc drift**: phantom `consoleDrawText()` in GETTING_STARTED (won't
  link) → `textModeInit`/`textPrintAt`; stale `oamSet` framesize warning removed;
  `oamSet` signature corrected in TROUBLESHOOTING + tutorials (was the old
  PVSnesLib arg order); `types.h`/`CLAUDE.md` int/long sizes corrected to the
  post-A1 truth (int=2, long=4); example counts → 56; `sa1_hello` comment
  0xA1→0xA5; `video.h` stale `@todo` removed.
- `simple_sprite` init order (`setMode` moved before VRAM/OAM setup); raw `0x40`
  H-flip → `OBJ_FLIPX` across likemario/koopatroopa/slopemario.

### Tooling
- New doc sentinels: phantom-API detector (GETTING_STARTED/TROUBLESHOOTING) and
  `check_doc_render.py` (scans the generated HTML for un-rendered Markdown);
  example-count drift check extended to onboarding docs. All wired into CI.

## [0.22.0] — 2026-06-23

Developer-experience & test-infrastructure release. **No lib / compiler / runtime
API changes** — those shipped binaries are unchanged from v0.21.3; the gains are
new devtools, a major test-harness expansion, and two examples migrated to a
generated VRAM layout (which render pixel-identically).

### Added
- **VRAM layout tooling** (`devtools/vram_layout/`): a declarative per-example
  `vram.spec` → an OR-Tools CP-SAT solver generates a committed `vram_map.h` of
  base addresses (aligned, non-overlapping, fit, min high-water). ortools is
  generation-only (pinned seed → reproducible); the build consumes the committed
  header and never needs it.
- **VRAM linter** (`make lint-vram`, CI): pure-stdlib gate that flags a misaligned
  BG/sprite base (silently masked by the hardware → wrong tiles) and validates each
  committed `vram_map.h` against its `vram.spec`. Catches the silent-failure class
  statically; never false-fails.
- **Test-harness expansion** (luna): coprocessor-execution proof (Super FX / SA-1
  instruction traces) with SA-1 chip→I-RAM handshake assertion; SRAM battery
  power-cycle round-trip; debug-channel runtime harness (`SNES_NOCASH` / `SNES_ASSERT`
  → `--nocash-out` / `--wdm-out`); SNES Mouse and Super Scope input probes; and a
  VRAM-DMA **timing-safety** check (`force_blank`) that asserts zero VRAM writes
  during active display — the #1 silent failure, testable for the first time.
  Functional probes 10 → 14.

### Changed
- **Test backend**: pinned luna `v1.0.0` → `v1.1.0` (peripheral input + `force_blank`
  trace column, both from our RFE). No rendering drift (visual regression 56/56).
- **Examples**: `collision_demo` and `metasprite` migrated to the generated
  `vram_map.h` (single source of truth, no magic numbers); identical rendering,
  ~24 KB / ~16 KB of VRAM high-water reclaimed respectively.

### Docs
- `gh` auth-via-`.env` convention note (`.claude/notes/conventions/`).

## [0.21.3] — 2026-06-22

Maintenance release — test-backend pin bump only. **No API / library / runtime /
compiler changes**: the shipped compiler, library and example ROMs are unchanged
from v0.21.2. This release captures the luna v1.0.0 migration on `main`.

### Changed
- **Test backend**: pinned luna bumped `v0.3.2` → `v1.0.0`
  (`tools/luna-test/luna.version`). The v0.3.x releases were removed upstream,
  which broke CI; v1.0.0 is CLI-compatible and produces no rendering drift
  (visual regression 56/56). Affects the dev/CI test harness only.

### Internal
- A6 (24-bit far-pointer deref) chantier investigation recorded across attempts
  #1–#3: a real latent `Ocopy` high-half bug was fixed, but the deref-far
  approach needs bank-byte preservation across *every* `Kl` move site; two
  strategic options documented for a future attempt. Not shipped (develop clean,
  WIP preserved on `wip/a6-deref-attempt3`). See
  `.claude/notes/chantiers/32bit_pointers_a7_a6_b1_b2.md`.

## [0.21.2] — 2026-06-22

Patch release — one compiler correctness fix (32-bit arithmetic) plus the
test-infrastructure that found it. No API / library / runtime changes.

### Fixed
- **Compiler (32-bit `Kl`)**: arithmetic shift-right of a *negative* 32-bit
  constant was constant-folded with the sign dropped — `(s32)-256 >> 4` gave
  `0x0FFFFFF0` instead of `0xFFFFFFF0`. QBE's fold pass evaluated the `Kl` shift
  as 64-bit on a zero-extended constant; on w65816 `Kl` is 32-bit, so `Osar` now
  folds with 32-bit signed semantics (`compiler/qbe/fold.c`). (chantier A7)

### Added
- 32-bit (`s32`/`u32`) **runtime-correctness fixture**
  (`devtools/compiler-tests/runtime/a7_32bit/`, 18 cases checked via
  `luna state --assert`), wired into `make tests` and CI — a permanent codegen
  gate the static C→ASM checks can't provide (they pass even on broken 32-bit
  arithmetic).

## [0.21.1] — 2026-06-22

Test-infrastructure hardening + cleanup release. No SDK API, library, compiler,
or runtime changes — existing ROMs build byte-for-byte the same.

### Added
- luna test-harness probes for coverage axes the old harness never had:
  **audio** (SNESMOD SPC voices + PCM RMS), **VRAM/ARAM content**
  (`--dump-vram`/`--dump-aram` upload verification), **decoded sprite structure**
  (`assets-dump` `oam.json`), and a **VBlank DMA-budget** estimate (`--dma-trace`).
  `probes/run_all.py` now runs 10 probes.
- `devtools/symmap/test_symmap.py` — regression test pinning `symmap.py
  --check-overlap` against committed `.sym` fixtures (broken map must fail, clean
  must pass); wired into the Lint workflow. Re-homes the orphaned
  `tools/debug-fixtures/`.
- `make test-wram` — local WRAM-state regression tool (`wram-trace` per-frame hash
  stream); not a CI gate (WRAM content, unlike the framebuffer, is not a luna
  cross-arch guarantee).

### Changed
- Bump the pinned luna binary **v0.3.0 → v0.3.2** (no rendering drift; visual
  baselines byte-identical). `LUNA_VERSION` now reads `tools/luna-test/luna.version`
  (single source of truth with `install-luna.sh`).
- Commit-scope lint accepts comma-separated scopes (`feat(compiler,lib): …`).

### Removed
- `.github/workflows/benchmark.yml` (disabled, contained dead `node
  opensnes-emu/...run-benchmark.mjs` steps; the cycle benchmark is re-homed to
  `devtools/cyclecount/bench.py` and runs in CI).

### Fixed
- Retire all dead references to the removed snes9x/Node test backend from active
  files: the Claude Code test hooks (now key on `make tests` + an
  `ALL CHECKS PASSED` banner), `compiler/PINS.md`, `opensnes_build.yml`,
  `lint_commits.py`, `abi_lint.md`/`memory_routing.md`, and the `ROADMAP.md`
  test-suite sections; `STRUCTURAL_DEFECTS.md` D1 marked resolved by the luna
  migration. luna is the sole automated test backend.

## [0.21.0] — 2026-06-21

Two headline efforts: a new **`fix32` fixed-point math module** and a full
**test-harness migration to luna** (a cycle-accurate native emulator). Plus
compiler `Kl` (32-bit) correctness fixes, the engine-review follow-ups, and a
procedurally-generated shoot-'em-up example.

### Added
- **`fix32` 16.16 fixed-point math** (chantier B5): `fix32Mul`, `fix32Div`
  (48-iteration long divide), `fix32Lerp`, `fix32Sin` / `fix32Cos`, the
  `snes/fixed32.h` header and inline helpers — backed by a new compiler **`Kl`
  (64-bit-class) return convention** for 32-bit results.
- **Easing LUTs** (chantier B6): `ease_in_quad` / `ease_out_quad`.
- New examples: **`fix32_orbit`** (B5 capstone), **`aim_target`** (fix32Cos/Sin
  alongside 8.8), and **`shmup_1942`** — a procedurally-generated vertical
  shoot-'em-up (cellular-automata archipelago + autotile resolver, vertical
  auto-scroll, 8-enemy spawn pool, AABB collision, BG3 HUD).
- `fadeOut` / `fadeIn` promoted into `snes/console.h`.
- **luna test harness** (`tools/luna-test/`, Python): `make tests` runs corpus
  liveness coverage + full-corpus visual regression (56 examples, keyed on luna's
  cross-arch-stable `--print-fbhash`) + functional probes (scripted input → WRAM
  via `--assert`) + audio checks (`--audio-out` + SPC voices) + WRAM-state
  regression (`wram-trace`) + VRAM/ARAM content + decoded sprite structure + the
  SDK `SNES_ASSERT`/WDM assertion oracle (`--wdm-out`).
- `scripts/install-luna.sh` — fetch + SHA-256-verify the pinned luna binary
  (v0.3.0); honours `$LUNA_BIN` for a local build.
- Re-homed compile-time checks off the removed emulator submodule:
  `devtools/compiler-tests/` (cc65816 C→ASM pattern checks, `make test-compiler`,
  declarative `.checks` DSL) and `devtools/cyclecount/bench.py` (cycle-count
  benchmark, `make bench`).

### Changed
- `dmaCopyVram` / `dmaCopyCGram` read the source bank from the caller's `Kl`
  pointer — DMA sources (and map data) can now live outside bank $00.
- Test harness migrated to **luna**: removed the `tools/opensnes-emu` submodule
  (snes9x libretro WASM core + Node runner + vendored Mesen2) and the
  dual-baseline (`.bin` / `.mesen2.bin`) scheme. CI drops emsdk / WASM build /
  Node-core / Mesen2 / xvfb / the ubuntu-22.04 pin. luna runs SA-1 / Super FX /
  DSP-1 natively, so the chip-ROM Mesen2 side channel is gone.
- `hdma_wave` moves its 7 KB tables out of `main.c` (bank $00 hygiene).
- Docs swept off snes9x/Mesen2 onto the luna harness; `KNOWN_LIMITATIONS.md`
  "snes9x can't detect the GSU" entry resolved (luna runs it natively).

### Fixed
- **Compiler `Kl` (32-bit) codegen**: `__mul32` alias dropped the bank byte
  (broke pointer / board indexing); `qbe` `Kl` shift-by-constant now spills
  `low_orig` before the `asl` chain; `mul32` / `div32` export fixes (the
  `tcc_[us]divmod32` alias trap, spurious `.EXPORT` in FREE sections).
- **Map data outside bank $00** (chantier B1, first milestone): the map-scroll
  path honours the layer bank, so large maps no longer silently read garbage.
- `oamSet` signature corrected in `compiler/ABI.md` (P0-1), now guarded by a
  doc-drift check.
- SA-1 SIWP/CIWP power-on polarity documented (kept `$FF`; P1-3).
- `shmup_1942` rendering fixes (bullet spawn / reach-top, BG scroll direction,
  enemy sprite V-flip).

### Performance
- `shmup_1942` writes player / bullet OAM directly, dropping per-frame `oamSet`
  overhead and fixing scroll lag.

## [0.20.0] — 2026-05-17

Chantier A1-followup — `long` arithmetic flows through 32-bit Kl
codegen end-to-end. Closes the silent-truncation hole left by
chantier A1 (v0.17.0): `int = 2`, `long = 4` shipped the SIZE alignment,
but cproc kept mapping 4-byte non-float to QBE class `'w'` (Kw,
16-bit), so every `long` operator was being silently truncated to
16 bits. The five reproductions documented in
`.claude/notes/chantiers/a1_followup_long_is_kw.md` — `(s32)40 << 16`
folding to zero, `s32 *= s32` routing through `__mul16`, etc. — are
now all closed.

Beyond the chantier, the commit-message linter was simplified: the
first-letter-of-description case check was overkill (Conventional
Commits doesn't mandate case) and just made external contributors
fight regex exceptions for acronyms like `OAM` or `A1-followup` for
no real win. Dropped entirely.

### Compiler

- **chantier A1-followup (cproc `c755269`, qbe `58ab12a`).** Cproc
  flips `qbe.c:215`: 4-byte non-float types return QBE class `'l'`
  (Kl) instead of `'w'` (Kw). QBE's w65816 backend gains the Kl
  handlers that were missing before this release — Kw paths existed
  for pointers (A6 chantier) but never for the rest:
  - `Oand`, `Oor`, `Oxor` Kl: independent low + high half operations.
  - `Oneg` Kl: two's complement on 32 bits with carry propagation.
  - `Oshl`, `Oshr`, `Osar` Kl: cross-half `asl`+`rol` (or `lsr`+`ror`,
    or `cmp #$8000`+`ror`) bit-by-bit for the 0 < cnt < 16 zone;
    XBA shortcut for cnt ≥ 8 within the high-half-only zone.
  - `Omul` Kl via `__mul32`: 10 hardware 8×8 partial products
    (4 for the full `a_lo * b_lo`, 3 each for the low-16 of the two
    cross terms). Constant-folding shortcuts for `*0`, `*1`,
    `*2^k`.
  - `Odiv` / `Oudiv` / `Orem` / `Ourem` Kl via `__sdivmod32` /
    `__udivmod32`: 32-iteration restoring shift-subtract,
    dividend-doubles-as-quotient trick. Signed wrapper handles
    abs() + sign restore (C99 truncation toward zero). Constant
    fast path for `/1` (identity copy) which cproc emits for every
    `u8*` ptrdiff.
  - 12 `Ocmp*l` (eq/ne/slt/sle/sgt/sge/ult/ule/ugt/uge) Kl:
    cascade high-half compare → low-half on equality. The high
    test honours signed vs unsigned per opcode; the low test is
    always unsigned because the low half is the less-significant
    chunk once the high half has decided the sign.
  - `Oextsw`, `Oextuw` Kl: already shipped in A6+A7; no change.
- **`can_be_frameless` Kl gate.** Any Kl temp in a function forces
  a real local frame. The pre-fix alias / dead-store / dead-retval
  shortcuts were Kw-only (a single 16-bit slot) and would have
  landed the high half of any Kl pair on top of the JSL return PCH,
  reproducing the `framesize=0 + Kl` synthetic bug detected during
  the chantier's Session 3-5 IR tests.

### Library

- **`lib/source/mul32.asm` and `lib/source/div32.asm` (new files).**
  Runtime helpers for the truncated 32-bit multiply and the
  signed/unsigned 32-bit divmod. Pinned to `BANK 7 FREE` so the
  ~500 bytes don't displace const strings out of bank 0 in the
  tighter examples. Linked unconditionally via `make/common.mk`
  (`MUL32_OBJ`, `DIV32_OBJ`) after the C objects, so cproc-emitted
  SUPERFREE sections claim bank 0 first.

### Build / CI

- **`devtools/lint_commits.py`: drop case-of-first-letter check.**
  The lowercase-first rule was an Angular convention, not from
  the Conventional Commits spec. Enforcing it just made contributors
  fight regex exceptions for every acronym / chantier identifier
  (`OAM`, `C99`, `A1-followup`, `A6.13`, ...) without buying
  anything real. 30 lines and 2 regexes deleted. `.claude/rules/commits.md`
  updated to match.

### Tests

- **Baselines refreshed for hdma_helpers + sa1_starfield.** The
  post-flip codegen shifted code sizes enough to cause a per-scanline
  HDMA timing artifact in these two examples (1581-pixel and
  similar drift respectively). The lib/example asm for these paths
  contains no `__mul32` / `__[s]divmod32` calls — the diff is a
  code-size shift propagating to frame timing, not a math bug.
  55 other `.meta` files updated to record the new ROM SHAs (all
  visually within tolerance).

### Documentation

- **`compiler/ABI.md`** — Type-sizes table gains an IR-class column.
  Pointer row corrected (`8 in IR` → `4`, post-A6+A7). `long / s32 /
  u32` annotated Kl with pointers to the backend handlers. The
  32-bit return values section rewritten to reflect actual conventions:
  compiler-generated `long`-returning functions convey only A=low
  across the call (latent issue), runtime helpers use named scratch
  slots (`mul32_hi`, `div32_qh`, `div32_rl`, `div32_rh`).
- **`KNOWN_LIMITATIONS.md`** — `int`/`long` entry now covers both
  sizes (A1) and semantics (A1-followup). Pointer entry 🟡 → 🟢
  (A6 already shipped in v0.19.0). New 🟡 entry calls out the
  latent C-function-returning-long high-half loss; mitigation today
  is "don't write that pattern".

### Tooling / Infrastructure

- **`compiler/PINS.md` bumped** to cproc `c755269` (12 patches, was 11),
  qbe `58ab12a` (49 patches, was 42). New `fix/a1-followup-long-kl`
  branch on the cproc fork carries the one-line class flip; the
  qbe branch (still `fix/a6-a7-leaf-opt-kl-frameless`) gained 7
  patches: bitwise/shift/mul/div Kl handlers, 12 compare cascade,
  `can_be_frameless` Kl gate, and the post-flip /1 / pow2 perf
  shortcuts.

---

## [0.19.0] — 2026-05-15

Chantier A6+A7 — full 24-bit pointer ABI + Kl pair lowering. The QBE
w65816 backend now treats pointers as 4 bytes (24-bit address +
1 pad) end-to-end instead of the legacy 8-byte Kl slot. cproc was
already aligned on `int = 2`, `long = 4` (chantier A1, v0.17.0); this
release completes the ABI by closing the pointer-size cascade that
A1 deliberately left for a dedicated chantier. Subsumes catalogue
defects B1 / B2 / B3 / B4 (the bank-`$00`-strangled chip-ROM and
mode-7 cases all stem from the 16-bit-pointer assumption in lib ASM).

Beyond the chantier, this release ships a three-layer defensive
infrastructure against the kind of retrofit miss that produced the
post-merge `hdma_gradient` regression: a build-time ASM↔C-signature
ABI lint, five runtime probes via mesen2-rpc, and documented
opt-out conventions. The published documentation site nav was also
broken (empty sidebar + TOC) — caused by missing jQuery/clipboard
script tags in the custom Doxygen header; fixed here.

### Compiler

- **chantier A6+A7 (cproc `6bdd923`, qbe `179676e`).** Cproc reduces
  pointer storage to 4 bytes (`type.c`: `mkpointertype` size/align
  8/8 → 4/2). QBE's w65816 backend adds full Kl pair lowering: 4-byte
  data-section emit (`dtype_size[DL]`), byte-accumulator param loading
  in `w65816_abi` (A6.10), Kl pair add/sub with carry/borrow
  propagation (A7.5), Kl pair load/store (A7.3/4), indirect-call bank
  byte preservation (A6.6/9/11) with large-frame indirect addressing
  (A6.8 — already shipped in v0.18.0). The acache_invalidate fix in
  `emit_store_high` is load-bearing: every `arr[i]` loop in the SDK
  was freezing at index 0 because Oextuh-Kl left the A-cache tagged
  stale across the high-half store. Diagnosed via Mesen2 step-through
  on `text/hello_world` (single-step until the loop counter showed
  `$AD17`, traced the codegen back to one cache-set without
  invalidate). See `.claude/notes/chantiers/a6_a7_unified_audit.md`
  for the full 5-session diagnostic history.

### Library

- **Lib ASM pointer-ABI rework (3 sites).** The lib hand-ASM helpers
  that take `u8 *` params have updated stack offsets and now read the
  bank byte directly from the new 4-byte pointer at offset +2 instead
  of the legacy `cmp #$80` heuristic. Sites: `lib/source/map.asm`
  `mapLoad` (mapscroll / slopemario / tiled / mapandobjects fixed),
  `lib/source/hdma.asm` `hdmaSetup` (gradient_colors / parallax_scrolling
  / transparent_window / window fixed), `lib/source/dma.asm`
  `dmaCopyVramMode7` (mode7_perspective fixed). 9 examples back to
  visual-regression-passing under the new ABI.

### Build / CI

- **`BANK0_FAIL_THRESHOLD` 16 → 8** (`make/common.mk`). The 4-byte
  pointer push at every lib call site burns 2 bytes of ROM per
  pointer-arg per call relative to the pre-A6 16-bit ABI. The three
  most pointer-heavy examples (likemario, tetris, mapandobjects)
  consume enough extra const space that the previous 16-byte
  ratchet failed the build. The new 8-byte threshold sits just below
  the new minimum (12 bytes free) — clawing it back to 16 is a
  follow-up chantier (lib code-size optimisations or routing the
  canonical force-emit anchors to a non-bank-`$00` section). See
  `.claude/rules/bank0_budget.md` for the full policy.

### Tests

- **`basics_random` baseline regenerated.** The example seeds its
  PRNG in `consoleInit()` from `REG_OPHCT | (REG_OPVCT << 8)`. A6's
  compiled-code-size shift moves the cycle at which the read happens,
  which deterministically rolls the seed. New baseline captures the
  new seed's sequence — no example bug captured (both pre- and
  post-A6 render correct rand() output, just from different starting
  seeds). Per `BASELINES.md`, this is the canonical
  rom_sha256-drift-signal regeneration.

### Tooling / Infrastructure

- **`compiler/PINS.md` bumped** to cproc `6bdd923` (11 patches, was 10),
  qbe `7a7f77f` (42 patches, was 36). The qbe count grew because the
  v0.18.0 hardening (open_memstream removal, macOS arm64 SIGBUS fix,
  Windows MinGW `__has_include` guard, diagnostic crash handler) plus
  the chantier A6+A7 atomic commit and the post-merge `leaf_opt` Kl
  frameless fix add 6 patches between the two pre- and post-A6 SHAs.

### Library

- **`lib/source/hdma.asm` post-merge retrofit (commit `c5689f5`).**
  `hdmaSetupBank` and `hdmaSetTable` were missed in the A6.12 lib
  retrofit pass. Their stack-offset reads still assumed the legacy
  2-byte pointer layout, which under the new 4-byte ABI sent
  `hdma_gradient` into a frozen state (gradient never animated under
  the **A** button input). Offsets shifted by +2 in `hdmaSetupBank`,
  channel offset by +2 in `hdmaSetTable`, and the bank-byte read
  switched to an explicit `lda 7,s` instead of the pre-A6 heuristic.
  Diagnosed by visual diff on the live ROM in Mesen2 — caught after
  the A6+A7 main merge, before users hit it.

### Tooling / Infrastructure

- **`devtools/check_asm_abi.py` — build-time ABI lint.** Parses C
  signatures in `lib/include/snes/*.h`, computes expected `lda N,s`
  offsets under the cc65816 L-to-R + post-A6 4-byte-pointer ABI, and
  flags any `lda N,s ; commentNamingParam` whose offset disagrees.
  Catches the same class of bug as the hdma_gradient regression
  *before* it reaches Mesen2. Wired into `make lint-asm-abi` and
  `make lint`. The lint is also prologue-aware: `phb` shifts the
  arg base by +1, `phx`/`phy` by +2 (16-bit mode assumed), so
  functions saving extra registers are still verified instead of
  skipped. 68 functions covered today (up from 0 — they were
  previously classified as "custom CC, skip"). The `phd` instruction
  is the canonical marker for legacy PVSnesLib R-to-L conventions
  and remains skipped; a file-level `; lint-asm-abi: skip-file`
  marker covers the one file that uses the legacy ABI without
  `phd` (audio.asm). Full policy in `.claude/rules/abi_lint.md`.

- **`tools/opensnes-emu` — 5 functional probes via mesen2-rpc.**
  Runtime behavioural assertions complementing the static lint. Each
  probe loads a ROM in mesen2-rpc, runs N frames, and asserts on a
  specific lib effect:
  - `hdma.test.mjs` — DMA channel 6 DMAP/BBAD configured for HDMA
    after `hdmaSetupBank` runs.
  - `oam.test.mjs` — `simple_sprite` OAM entry at $7E:0300 has
    X=112, Y=95 (96-1 for the SNES PPU Y+1 quirk), tile=$10.
  - `dma.test.mjs` — `dmaCopyVram(font_tiles, 0, 144)` round-trips
    bytes ROM→VRAM byte-for-byte.
  - `cgram.test.mjs` — `dmaCopyCGram(bg_palette, 0, 4)` round-trips
    ROM→CGRAM.
  - `scroll.test.mjs` — `bgSetScroll(0, 0, 208)` writes the right
    arg to `bg_scroll_y[0]`, exercises a 3-arg-mixed-width signature.

  The probes share a `spawn-mesen.mjs` harness and run on TCP 9930
  one at a time (`--test-concurrency=1`).

- **`devtools/check_asm_abi.py` shift-aware extension.** Before this
  release the lint skipped every function whose prologue saved extra
  registers (`phb`, `phx`, `phy`, `phd`) by marking them `custom_cc`.
  Most of those 65 skipped functions actually use the standard
  cc65816 ABI — they just save the data bank or X/Y. The lint now
  tracks prologue pushes, adds the cumulative shift to the arg base,
  and verifies the offsets normally. `audio.asm` opts out via a
  file-level marker because it uses the legacy PVSnesLib R-to-L
  1-byte-packed convention. Coverage net: ~186 → ~254 functions
  verified, 22 explicitly skipped (audio.asm whole file), 128
  internal helpers with no public signature.

### Documentation

- **GitHub Pages nav + per-page TOC restored.** The published site
  at `k0b3n4irb.github.io/opensnes` had an empty left sidebar and
  empty TOC. Root cause: Doxygen 1.13's default header sources
  `jquery.js`, `dynsections.js`, and `clipboard.js` automatically,
  but the custom `docs/header.html` was dropping those tags
  silently. The inline `$(function(){initNavTree(...)})` then hit
  `$ is not defined`, JS halted, the sidebar container stayed empty.
  Fixed by sourcing the three scripts explicitly. The doxygen-awesome
  companion scripts (`darkmode-toggle.js`, `fragment-copy-button.js`,
  `paragraph-link.js`) had the same problem and are also now sourced.
  Validated end-to-end via jsdom probe: `#side-nav` content grew
  from 207 chars (empty container) to 10 486 chars (58 anchor links).

- **`docs/mainpage.md` — 8 missing tutorial `@subpage` refs added.**
  `tutorial_colormath`, `tutorial_dma`, `tutorial_hdma`, `tutorial_math`,
  `tutorial_mode7`, `tutorial_mosaic`, `tutorial_sram`, `tutorial_window`
  existed on disk but never appeared under the "Tutorials" mainpage
  section; they were flat top-level orphans in the nav. Now 18/18
  tutorials nest correctly.

- **`docs/EXAMPLES_BY_CATEGORY.md` and `docs/LEARNING_PATH.md` —
  broken `examples_enhancement_*` refs fixed.** Four IDs
  (`_sa1_hello`, `_sa1_starfield`, `_superfx_hello`, `_superfx_3d`)
  pointed to pages that never existed; the actual targets are under
  `examples_memory_*` and `examples_graphics_effects_*`. Eleven
  example pages that existed on disk but were never referenced are
  now `@subpage`'d: `aim_target`, `random`, `scene_stack`, `timer`,
  `controller`, `mapscroll`, `tiled`, `snesmod_music_hirom`,
  `snesmod_music_large`, `dynamic_metasprite`, plus the `superfx_3d`
  rename.

- **`docs/doxygen_md_filter.py` — auto-`@subpage` of child READMEs.**
  When the filter processes a `README.md`, it now scans for sibling
  sub-directories with their own `README.md` and injects `\subpage`
  directives at the end of the rendered page. This means category
  hubs (`examples_audio`, `examples_basics`, etc.) automatically
  become parents of their child example pages in the nav tree
  without manual edits per file. Top-level orphan count: 41 → 5
  (Mainpage, Topics, Classes, Files, Todo — all legitimate Doxygen
  built-ins).

- **`docs/Doxyfile` — `*/README_TEMPLATE.md` excluded.** Was leaking
  as a phantom `[Example Name]` top-level page in the nav.

- **`lib/include/snes/audio.h` — legacy ABI `@warning` block.**
  The 26 functions in `lib/source/audio.asm` use PVSnesLib's
  right-to-left push + 1-byte-packed u8 args, not cc65816's
  left-to-right + 2-byte-slot ABI. Multi-arg and pointer-taking
  audio functions called from C operate on garbage. Zero examples
  call this API today (SNESMOD is the working audio path), so the
  bug class has been latent. The header now lists which functions
  work (zero-arg + single-u8-arg, by coincidence) vs which are
  broken, and points users at SNESMOD. Detailed analysis in
  `.claude/notes/tech/audio_legacy_pvsneslib_abi.md`. Audio
  rewrite/shim deferred to a dedicated chantier when a caller
  surface appears.

## [0.18.0] — 2026-05-14

Function inlining + lib retrofit release. The compiler gains end-to-end
`inline` keyword support (chantier "function inlining"), plus deferred
asm emission with consumption-aware standalone suppression that makes
C99 cross-TU inline patterns work end-to-end. Sixteen lib helpers are
retrofitted to use the new inline path — 55 examples now inline
`setScreenOn()`, 22 inline `setScreenOff()`, plus scattered wins on
`fixCos`, `textSetPos`, `colorMathInit/Enable`, etc.

Total cycle reduction vs PVSnesLib+816-opt improves from **−29.1 %**
(v0.17.0) to **−32.2 %**. The previously-regressing `call_chain` bench
recovers from +31.9 % to −59.6 % vs PVSnesLib+opt. The catalogue's A4
(oamSet perf cliff) is acknowledged RESOLVED — fix actually shipped
2026-03-03 via tactical ASM rewrite (commit `39dbff8`) but the doc
hadn't been updated. B4/B6/D1 also receive partial-shipped status
updates after a full catalogue audit. No public API breakage.

### Compiler

- **Function inlining end-to-end (chantier "function inlining").** The
  C `inline` keyword now drives a real qbe inline pass. cproc forwards
  the keyword as a QBE IR linkage hint
  (`Lnk.inline_hint`); qbe's new `inline.c` records each eligible
  callee's body during the per-function pipeline and splices it into
  caller IR at every direct call site. Eligibility heuristic:
  linear-flow (no phi, no conditional jumps), no nested `Ocall`, no
  `Oalloc*`, ≤ 8 IR instructions (overridable via
  `CC_INLINE_MAX_INSTR=N` env var). The bench's `helper(helper(x))`
  pattern was the motivated target — its previous +31.9 % regression
  becomes a −59.6 % win once `helper` is marked `static inline`.
  Reference: `compiler/qbe/inline.c`, qbe SHAs `13d9b14`, `29b0941`,
  `77d07a0`. 410/410 quick suite green throughout.

- **Deferred asm emission + consumption-aware standalone suppression
  (cross-TU enabler).** The earlier inline ship only worked within a
  single TU (user `static inline` helpers in their own .c file). For
  lib helpers defined in a header to inline at example call sites, the
  body needs to reach the inline pass without the linker seeing
  duplicate symbols. Implementation: cproc/decl.c always emits inline
  bodies into IR (drops the C99-inlinedefn-based skip);
  qbe/main.c buffers each function's asm in an `open_memstream` buffer
  during the per-function pipeline; qbe/inline.c tracks
  per-fn `n_direct` / `n_inlined` / `n_declined` / `n_indirect`
  consumption counters; `flush_pending()` skips the standalone in two
  cases — (a) every direct caller in this TU inlined and no indirect
  refs, or (b) the function isn't used in this TU at all
  (header-only inclusion). A canonical TU forces the standalone via a
  data-section indirect reference (`void(*const force_emit)(...) = fn;`).
  Reference: `compiler/qbe/main.c`, `compiler/qbe/inline.c`,
  qbe SHA `988073d`, cproc SHA `42a5c46`.

- **`CC_INLINE_MAX_INSTR` default bumped 8 → 16 (qbe `2eb9f53`).**
  Opens wave 4 candidates (`textSetPos` 10, `fixCos` 11,
  `colorMathEnable` 14). Dormant for the 12 helpers shipped with the
  ≤ 8 cap. No measurable bank-$00 ROM regression.

- **A6.8: large-frame indirect addressing + Kl slot widening
  (qbe `4aa4989`).** Standalone prerequisite for the in-flight A6+A7
  atomic patch (full pointer ABI, parked on
  `wip/a6-a7-atomic-v3`). Adds an indirect addressing fallback for
  functions whose stack offsets exceed the 8-bit `<N>,s` cap, and
  widens Kl temp slots to 4 bytes. Not user-visible in v0.18.0; lays
  the groundwork.

### Library (perf retrofits)

Sixteen lib helpers now ship with the C99 `inline` pattern. Every TU
that includes the right header inlines the body at direct call sites;
the canonical `.c` file emits one standalone fallback for fn-pointer
callers. Per-call overhead drops from ~28 cycles (JSL + RTL + prologue
+ epilogue) to zero.

- **Wave 2 (`ebcf428`):** `getBrightness`, `mosaicInit`,
  `hdmaWaveSetSpeed`, `scopeCalibrate`, `colorMathInit`,
  `colorMathDisable` — 6 helpers, ~28 cycles each per call, 2 with
  active example callers (scopeCalibrate × 1, colorMathInit × 1).
- **Wave 3 (`98387fd`):** `setScreenOn` (the big win — **55** example
  callers), `getFrameCount`, `gsuInit`, `scopeSetHoldDelay`, plus the
  static-inline of `mosaic_update_register` collapsing 5 intra-TU
  callers. Saves ~1736 cycles across the SDK on the dominant
  setScreenOn path.
- **Wave 4 (`85416eb`):** `textSetPos`, `fixCos` (+ paired `fixSin`),
  `colorMathEnable` — the 9-14 IR-instr range opened by the threshold
  bump. ~112 cycles saved across 3 examples (random, aim_target,
  transparency).
- **Wave 1 was `setScreenOff`** (already in the deferred-emit ship,
  `2b4e2f8`) — 22 example callers, the first symbol exercised end-to-end.

Total: ~2700+ cycles saved across the SDK at typical example call
sites. The pattern (`extern u8 state;` in header + `inline void
fn(...)` body + `void(*const force_emit)(...) = fn;` in canonical .c)
is reusable; the audit in
`.claude/notes/chantiers/lib_inline_retrofit_assessment.md` documents
when to retrofit and when to skip.

### Performance

- **Cycle benchmark TOTAL: 1341 → 1282** (-4.4 % vs v0.17.0).
- **vs PVSnesLib+816-opt total: -29.1 % → -32.2 %** (improvement of
  +2.3 percentage points).
- **`call_chain` recovered**: 62 → 19 cycles, going from
  +31.9 % vs PVSnesLib+opt to −59.6 % (a 91-point swing on the
  benchmark's previously-worst case).
- **`helper` standalone eliminated post-deferred-emit**: counted as
  0 cycles in the table with a (†) footnote — its body lives
  exclusively inside `call_chain` after the inline splice. ROM-size
  win (~6 bytes per fully-inlined helper).

The wins concentrate on:
- direct-call elimination at user call sites (every `setScreenOn()`
  collapses to two register writes)
- dead-standalone removal (fully-inlined helpers don't bloat ROM)
- `call_chain`-shaped patterns (small wrapper around a small wrapper)

### Docs / Catalogue

- **A4 oamSet perf cliff RESOLVED 🟢** (commit `5fdad3b`). The fix
  actually shipped 2026-03-03 (commit `39dbff8`) via ASM rewrite in
  `lib/source/sprite_oamset.asm` — framesize dropped from 158 to 0.
  Catalogue + `KNOWN_LIMITATIONS.md` had 2.5 months of drift; both
  updated to reflect reality.
- **B4 (`hdmaSetup` hardcodes bank $00) → 🟡 partial** (commit
  `6db13fc`): `hdmaSetupBank()` shipped, 1/5 HDMA examples migrated.
- **B6 (no atan2/sqrt/pow) → 🟡 partial**: `sqrt16`, `atan2_8`,
  `fixSqrt` shipped (algorithmic, not LUTs as originally proposed,
  but functional). `pow_lut` is the remaining gap.
- **D1 (snes9x doesn't detect GSU) → 🟡 partial**: Mesen2 visual
  regression phase shipped and CI installs Mesen2 unconditionally
  (ubuntu-22.04 ABI match + xvfb for headless). The "fail-build if
  Mesen2 unavailable" criterion is met for CI; local dev still skips.
- **Function inlining audit + assessment docs**: full Phase 0/1
  audit before implementation, post-mortem on the C99 strict pattern
  attempt, then the unblock when the cproc/QBE deferred-emit chantier
  shipped. References:
  `.claude/notes/chantiers/function_inlining_audit.md`,
  `.claude/notes/chantiers/lib_inline_retrofit_assessment.md`.
- **Wip/* branch policy** (`060dd93`): squash-merge then delete.
  Documented in `.claude/rules/release.md` after a cleanup pass found
  5 stale `worktree-agent-*` branches + 1 `backup/develop-pre-reword-*`
  living forever.
- **Bench re-baselined twice** (`ccaf9c7` post-v0.17.0, `022572d`
  post-deferred-emit). `docs/BENCHMARK.md` reflects the
  −32.2 % current state with full per-function breakdown.
- **A6+A7 chantier audit phase complete** (multiple docs commits):
  the in-flight full-pointer-ABI chantier has its 9-site atomic patch
  plan, the empirical findings of two implementation attempts, and a
  replay script. Parked on `wip/a6-a7-atomic-v3` for the focused
  session that needs Mesen2-debugger access to close the residual 46
  failures.

### Tooling / Infrastructure

- `compiler/PINS.md`: bumped to qbe `5c23467` (40 patches, was 31)
  and cproc `42a5c46` (10 patches, was 8). Patch count grew across
  the function-inlining + deferred-emit chantier; trend opposite of
  catalogue A5's "shrink toward upstream" goal but the patches are
  load-bearing for the perf wins.

### Build & CI (cross-platform unblock)

Three pre-existing CI failures resolved end-to-end in the same session
that closed the v0.18.0 release. The Linux + Functional Tests jobs
were already green; macOS arm64 and Windows MinGW had been red for
many commits. All four jobs now green on the release tip.

- **`open_memstream` dependency removed (qbe `eaf6116`).** The earlier
  deferred-emit chantier bufferised each function's asm output through
  `open_memstream` (POSIX 2008). MSYS2's UCRT does not ship the call,
  which broke the Windows qbe build before any lib compile ran.
  Redesigned as a 2-pass parse: pass 1 (in the parse callback) runs
  SSA cleanup + `inline_record` and collects every Fn; pass 1.b runs
  `inline_check` module-wide so consumption counters see every TU
  caller; pass 2 finalises and emits, skipping fully-consumed inline
  bodies. The w65816 backend writes `w65816_alloc_size[]` /
  `w65816_alloc_slots` globals in abi0 and reads them at emit time;
  the new abi0 snapshots the per-fn state into a side list and emit
  restores it, since the 2-pass model separates abi0 from emit by
  many other functions' processing. Output is byte-identical to the
  previous design across the lib and every example.

- **macOS arm64 SIGBUS fixed (qbe `444edea`).** `parsedat` declared
  `Dat d` on the stack and fired the `DStart` callback before
  `d.isref` / `d.u.ref.name` were written, so those bytes carried
  stack garbage. The OpenSNES `data()` callback's `inline_record_dat_ref`
  lookup then dereferenced a junk pointer at `r->n_indirect` (offset
  108 in `InlRec`). Linux x86_64 / aarch64 typically saw zero bytes
  there so the surrounding `if (d->isref && d->u.ref.name)` test
  failed harmlessly; macOS arm64's stricter stack hygiene produced
  reliable SIGBUS when compiling `background.c`. Fix gates the call
  on `d->type != DStart && d->type != DEnd` so the `isref` byte is
  only trusted once parsedat has written it. Diagnosed with a
  SIGBUS/SIGSEGV crash handler added in qbe `4de6a97` that dumps
  `backtrace_symbols_fd()` output — pinpointed the crash site without
  arm64-Apple hardware.

- **Windows MinGW build fix (qbe `5c23467`).** The diagnostic handler
  used `<execinfo.h>`, which is glibc + Apple libsystem only. MinGW
  does not ship it, so the Windows qbe build aborted before any lib
  compile reached the cproc segfault retry. The handler is now guarded
  behind `__has_include(<execinfo.h>)` so it compiles out on Windows;
  Windows crashes continue to be diagnosed via
  `msys2_cproc_diagnostic.yml` + the retry loop in
  `opensnes_build.yml`.

## [0.17.0] - 2026-05-09

Compiler & runtime hardening release. Five structural defects from
the post-audit catalogue (`.claude/STRUCTURAL_DEFECTS.md`) close in
this version (A1, A2, A3, E1, E2), one new lib API ships (B6 — sqrt
and atan2), and the build infrastructure gains two permanent guard
rails (NMI/WRAM port race lint, hard cycle-count CI gate). Three
🔴 silent failures from the catalogue downgrade to 🟢 (caught at
build time). Public API is unchanged across the upgrade.

### Compiler

- **`volatile` is preserved through the cproc → QBE pipeline
  (chantier A2).** Pre-A2 cproc literally discarded the
  `QUALVOLATILE` qualifier with `(void)(tq & QUALVOLATILE);` and a
  `TODO` comment. As a result every C `volatile` load/store entered
  QBE IR as a plain operation, and the load-forwarding pass coalesced
  redundant volatile reads through value-numbering — a silent
  miscompile that violated the C standard's "each access is a side
  effect" volatile semantics. Concrete pre-fix symptom:

      volatile unsigned short timer;
      unsigned short read_twice(void) { return timer + timer; }
                                              /* MUST emit 2 loads */

  Pre-A2 emitted `lda timer; sta cache; clc; adc cache` (1 load +
  1 cached read). Post-A2 emits two distinct `lda timer` loads as
  the standard requires.

  Fix touches three submodules: cproc threads `QUALVOLATILE` through
  `funcload`/`funcstore` and emits a `volat` IR keyword via a new
  `funcinst_volat` helper; QBE's `Ins` struct gains a `volat` field,
  the parser recognises a `Tvolat` token, and `loadopt` /
  `promote` / `gcm` skip volatile-tagged instructions; the
  opensnes-emu test fixture's `volatiles` phase now asserts load /
  store counts on `test_volatiles.c` rather than just compilation
  success. PINS.md lists 8 cproc patches and 29 qbe patches now.
  KNOWN_LIMITATIONS, CLAUDE.md, the compiler/new_example rules, and
  compiler/ABI.md updated to remove the historical "use globals
  instead of `volatile`" warnings — the lib still favours globals
  for NMI handshakes for cycle-cost equivalence, not because
  `volatile` is broken.

- **`int` is now 16 bits, `long` is now 32 bits on cc65816 (chantier A1).**
  cproc's IR was inherited as a host-target compiler with
  `sizeof(int) = 4` and `sizeof(long) = 8` hardcoded — wrong for a
  16-bit CPU. The QBE w65816 backend already treated these classes
  as 16/32-bit at emit time (per the `d929b94` patch's class `'l'`
  reinterpretation), so cproc's IR sizes were inconsistent with
  reality. Aligned in `compiler/cproc/type.c` (4 lines, 7 cproc patches
  total now). Bare `int counter` no longer silently produces 32-bit
  arithmetic — a documented `KNOWN_LIMITATIONS` 🟡 closes to 🟢. The
  benchmark suite shows `loop_sum` improved by 5 cycles per iteration
  (the only function in the suite using bare `int`); other functions
  are unchanged because the SDK convention is `u16` everywhere.
  Pointer size deliberately stays at 8 — its companion fix is the
  separate chantier A6 (pointer ABI + bank-byte preservation) tracked
  in the structural-defects catalogue. `compiler/PINS.md` now lists 7
  cproc patches; submodule bumped to `7f26c16`.

### Tests

- **Visual baselines for `basics/random` and `superfx_3d` regenerated** —
  both already documented as timing-fragile in `BASELINES.md`. The
  cproc compiler change shifts emitted instructions slightly, which
  cascades to a different RNG seed at frame 120 (random) and a
  slightly different rotation angle (superfx_3d). Pixel comparison
  catches any rendering-pipeline regression as before; the new
  baselines pass byte-by-byte under the rebuilt parent. Submodule
  `tools/opensnes-emu` bumped to `c440825`.

### Performance

- **`WaitForVBlank` no longer sets `oam_update_flag` unconditionally** —
  the runtime's NMI handler already conditionally skips the OAM DMA
  when the flag is clear, but `WaitForVBlank` was setting the flag on
  every call, defeating the optimisation. Sprite-idle ROMs (text-only
  games, paused screens, splash frames) now save ~4.3 K cycles per
  frame (the cost of the unnecessary 544-byte OAM DMA). The contract
  is now: **callers that mutate `oamMemory[]` directly must set
  `oam_update_flag = 1` themselves.** Every OAM-mutating function in
  `lib/source/sprite.c` and the `oamSetFast` / `oamSetXYFast` macros
  in `lib/include/snes/sprite.h` already set it; the documented
  user pattern (per `KNOWN_LIMITATIONS.md` "Performance traps") is
  unchanged. See `templates/crt0.asm:1184+` for the updated function
  comment naming the new contract explicitly.

### Library

- **`<snes/math.h>` ships `sqrt16`, `fixSqrt`, and `atan2_8`
  (chantier B6).** The "where is the target relative to me, and how
  far?" surface every 2D action game leans on now lives in one
  module:
    - `sqrt16(n)` — bit-by-bit integer square root, ~80 cycles, no
      LUT, result bounded to 255 (fits in `u8`).
    - `fixSqrt(x)` — 8.8 fixed-point sqrt; delegates to `sqrt16`,
      4 fractional bits of precision (capped intentionally —
      raising it requires 32-bit shift currently broken under the
      QBE 32-bit codegen gap, catalogue chantier A7).
    - `atan2_8(dy, dx)` — 8-bit angle of the `(dx, dy)` vector in
      the same convention as `fixSin` / `fixCos` (0=+X, 64=+Y, …).
      65-byte LUT covers the first octant, symmetry handles the
      other seven, scale-invariant reduction prevents the internal
      16-bit divide from overflowing on any 16-bit input.

  The `atan2_8 → fixCos / fixSin` chain is the canonical pattern
  for projectile aiming, pursuit AI, and "rotate sprite to face X".

  `lib/source/hdma.c` was using a private static `isqrt()` for the
  iris-wipe radius — promoted to `sqrt16` (same algorithm,
  identical results). `make/common.mk` adds the transitive dep
  `_DEP_hdma := dma math_sqrt`, which means hdma-using examples
  pull only the small sqrt module rather than the full math runtime.

- **`math.c` split into `math.c` + `math_sqrt.c`** (post-B6
  hygiene). The hdma → math transitive dep would otherwise have
  dragged the full sine LUT (512 B), atan LUT (65 B), and
  arithmetic helpers into every hdma-using example, even when the
  caller only needed `sqrt16` through hdma. Six examples reclaimed
  ≈ 1721 bytes of bank-$00 free space each (e.g. `parallax_scrolling`
  5628 → 7349, `gradient_colors` 12324 → 14045). Five examples that
  listed `math` in `LIB_MODULES` without using it had the dead
  listing removed (`window`, `parallax_scrolling`, `hdma_gradient`,
  `transparent_window`, `gradient_colors`). Public `<snes/math.h>`
  surface unchanged — the resolver flattens
  `_DEP_math := math_sqrt` so `LIB_MODULES := math` still pulls
  sqrt as a transitive dep.

### Examples

- **`examples/basics/aim_target` — interactive showcase for
  `sqrt16`, `atan2_8`, and trig chaining.** Player diamond at
  screen centre, X target follows the D-pad. Each frame the
  example recomputes `dx`/`dy`, calls `sqrt16(dx² + dy²)` for
  pixel distance, calls `atan2_8(dy, dx)` for the 8-bit angle,
  then calls `fixCos`/`fixSin` on that angle to read back the
  unit direction vector. Live readout panel above the play area
  shows DX, DY, DIST, ANGLE, COS(ANGLE), SIN(ANGLE) — all the
  numbers update as the user moves the target. First example in
  the suite that exercises fixed-point math at all.

### CI / build infrastructure

- **NMI / WRAM data port race lint shipped (chantier E1).** The
  SNES WRAM data port (`$2180`-`$2183`) shares state between the
  main thread and the NMI handler; if main-thread code is
  mid-sequence on those ports and an NMI fires whose callback also
  touches them, the address pointer is silently corrupted and the
  main thread resumes writing to the wrong location with no error.
  KNOWN_LIMITATIONS flagged the trap with a 🔴 severity tag and
  documentation-only mitigation. Now caught at build time:
  `devtools/check_nmi_wram_race.py` parses each example's
  `combined.asm` + `*.c.asm` intermediates, walks the call graph
  from every NMI callback root (NmiHandler + DefaultNmiCallback +
  every symbol passed to `nmiSet`/`nmiSetBank`), takes the
  closure, and fails the build if any reachable function writes
  to `$2180-$2183`. Wired post-link in `make/common.mk` next to
  the BANK0 ratchet. Bypass per-build with `SKIP_NMI_RACE_CHECK=1`.
  Severity downgraded 🔴 → 🟢. Regression suite
  `devtools/test_check_nmi_wram_race.py` (6 cases) wired into
  `.github/workflows/lint.yml` as the new `nmi-race-tests` job.

- **Cycle-count CI gate promoted from soft to hard (chantier E2).**
  The gate shipped 2026-05-08 in soft / comment-only form
  (commit `98d5014`); after 24 h of operation the threshold design
  held up and is now hard. Two-armed thresholds:
    - **Total cycles** > 5 % regression → fail (broad-drift catch)
    - **Per-function** > 25 % AND > 50 cycles absolute → fail
      (pathological catch — the combined percent + absolute floor
      avoids small-routine noise)

  Override path: include `Cycle-Regression-OK: <reason>` as a
  trailer in any commit in the PR range. The workflow scans
  `git log <base>..<head> --format=%B`; if the trailer is present
  the gate exits 0 even on a breached threshold (the comparison
  is still posted as a comment with a "regression overridden"
  header so the trade-off is visible to reviewers). Trailer
  preferred over PR labels (audit trail in git history forever),
  over comment markers (no audit trail), over self-service
  bypasses (anyone with push access can write the trailer).
  `devtools/cyclecount/cyclecount.py` gains `--fail-on-regression`
  (with tunable `--total-pct-limit` / `--fn-pct-limit` /
  `--fn-abs-limit`).

### Tests

- **`[KNOWN_BUG]` markers retired and `--allow-known-bugs` dropped
  from CI (chantier A3).** The compiler-test phase carried 7
  stale `knownBug()` calls escaping assertions for "TCO not
  implemented yet" and "A-cache through pha not implemented yet".
  Investigation compiled each fixture and inspected the generated
  ASM: every optimisation the markers gated had already shipped
  (TCO via C.1 / C.2.1 / C.2.2; A-cache through pha audited
  working in C.6; lazy `rep #$20` emission active). The
  assertions were drifting behind the codegen, not anticipating
  future work.

  The `compute_across_call.*leaf_opt=1` assertion in
  `nonleaf_frameless` was actively wrong — that function MUST
  get `leaf_opt=0` because `sum` is live across the call (the
  `all_calls_are_tail()` gate at `compiler/qbe/w65816/emit.c:2882`
  correctly returns 0 for it). Rewritten to check
  `forward_with_work.*leaf_opt=1` instead.

  7 `knownBug()` calls converted to `fail()`. CI workflow's
  `--allow-known-bugs` flag dropped. Real regressions on TCO /
  A-cache / lazy rep now hard-fail. The MSYS2-segfault paths
  remain — they only fire on Windows / MSYS2 builds and are
  orthogonal to the optimisation gaps.

- **`test_volatiles` phase strengthened from a smoke check to
  semantic assertions** (companion to chantier A2). Three reads
  of `hw_status` must produce ≥3 `lda hw_status` instructions;
  four writes of `hw_data` must produce 4 stores; the
  read-modify-write helper must produce 2 reads + 2 writes
  balanced. Pre-A2 the QBE loadopt pass coalesced the reads
  silently, and the test never noticed because it only checked
  compilation succeeded.

### Fixed

- **`examples/games/tetris` HDMA gradient migrated from channel 7 to
  channel 6** — the example was writing directly to `$4370`-`$4374`,
  reusing the channel reserved by the NMI's OAM DMA path. The conflict
  was masked at runtime by `WaitForVBlank`'s unconditional flag set
  re-programming channel 7 every frame; with the perf change above
  removing that unconditional set, the latent contract violation is
  exposed. Migrated to channel 6, which the lib does not reserve.

### Maintainer / docs

- **Strategic-defects catalogue promoted to
  `.claude/STRUCTURAL_DEFECTS.md`** (was previously scratch in
  `/tmp` between sessions). The file tracks effort estimates, risk
  profiles, an interaction matrix, sequencing paths, and
  investigation logs for every defect that requires a deliberate
  multi-day chantier. Lives under `.claude/` because it's
  maintainer-internal operational planning, not user-facing
  documentation — contributors looking at the project's headline
  state still read `KNOWN_LIMITATIONS.md` and `ROADMAP.md` at the
  repo root for the public severity tags. CLAUDE.md gets a
  "Strategic Planning" section pointing at the catalogue.

- **`A6` (pointer ABI + indirect-call bank-byte preservation)
  partial implementation reverted with full investigation log.**
  Attempted `A6.1` (Ostorel CAddr → memory bank byte) +
  `A6.3` (indirect call read bank byte from slot+2). The fix
  passed 404/404 tests by coincidence: `A6.1` writes the bank
  byte to a memory location, but `A6.3`'s read targets a stack
  slot — the two are disconnected at the QBE Oload boundary
  (Oload is 16-bit only on the w65816 target). Tests passed
  because all framework callbacks land in bank `$00` thanks to
  SUPERFREE placement, so `lda #$00` and `lda slot+2,s` happened
  to emit identical-effect code. Reverted; root cause documented;
  the full A6 chantier requires a coupled A6.4 / A6.5 / A6.6
  bundle (cproc pointer 8→4 + QBE Kl class 24-bit load/store +
  ASM site audit) before any of it can ship safely.

- **New defect `A7` logged in the catalogue.** Surfaced during
  B5 (`fixed32` 16.16) investigation: the QBE w65816 backend
  truncates *every* 32-bit arithmetic operation to 16 bits
  silently. `Oadd`, `Osub`, `Oshl`, `Oshr`, `Oneg`, and the
  `loaduw → __mul16` lowering all emit a single 16-bit
  instruction with no carry / borrow into the high half. A1
  shipped `sizeof(long) == 4` in cproc IR but did not extend
  the QBE backend to actually do 32-bit arithmetic — storage
  is right, operations are wrong. The lib's own `fixDiv`
  (`lib/source/math.c`) is broken for any dividend > 127
  because of this; no shipped example exercises that path so
  the bug is currently latent. Validated runtime with a
  reproduction ROM displaying five `BUG` rows for the four
  synthetic cases plus the lib `fixDiv` direct hit. Catalogue
  entry has the full investigation, the reproduction ROM
  spec, and the proposed A6.4-style fix surface (slot widening,
  `__mul32` runtime, carry-aware Oadd/Osub).

- **Visual baselines for `basics/random`, `games/tetris`, and
  `graphics/effects/superfx_3d` regenerated** — all three are
  documented timing-fragile examples (RNG seeded by `frame_count`,
  gameplay-driven state, continuously-animated rotating cube), and
  the one-cycle shift in `WaitForVBlank` produces small frame-120
  visual deltas that exceed the 50-px tolerance. Pixel comparison
  catches any rendering-pipeline regression as before; the new
  baselines pass the rebuilt suite cleanly. Submodule
  `tools/opensnes-emu` bumped to `0931e1a`.

## [0.16.0] - 2026-05-07

Framework trilogy completed. The two remaining "framework opt-ins"
promised by `PHILOSOPHY.md` (alongside D.1 `gameloop` in v0.15.2)
ship in this release: `<snes/asset.h>` for typed background and
tileset bundles (D.2), `<snes/scene.h>` for a push/pop scene stack
(D.3). An audit-driven cohesion pass aliases the gameloop config
type to the new `Scene` type, defers scene init to the next VBlank
dispatch (closing a silent VRAM-overrun footgun), and drops a
parallel macro API that did not justify its surface.

Two compiler fixes shipped along the way: chantier C.5 padded DL/DW
static init data emissions to match `dtype_size` (which unblocked
typed struct values with pointer fields), and the C.7
JSL-in-conditional codegen bug was confirmed silently fixed by an
earlier QBE chantier — the workaround macro is gone. Chantier P3.2
closed four residuals from the sprite/text duplication audit
(orphan font header, duplicated size-table macros, unused
`oamDrawMetasprite` API, "legacy" → "internal" naming).

### Added

- **`<snes/asset.h>` + `asset` library module (chantier D.2)** —
  typed `BgAsset` / `GfxAsset` value form for full background and
  tileset bundles, plus `bgLoad()` / `gfxLoad()` runtime functions.
  Constructor sugar via `DECLARE_BG_ASSET(name, color_mode, map_size)`
  / `DECLARE_GFX_ASSET(name, color_mode)` that expand to the six
  `extern` declarations and a populated `static const`. Symbol-naming
  convention: `<name>_tiles` / `<name>_pal` / `<name>_map` (each with
  matching `_end` siblings). Replaces the previous "declare six
  externs, compute three sizes, thread eight positional parameters
  through `bgInitTileSet`" boilerplate with a single typed value.

- **`<snes/scene.h>` + `scene` library module (chantier D.3)** —
  push/pop scene stack with `sceneRun(initial)`, `scenePush(next)`,
  `scenePop()`. `Scene` is two function pointers (`init`, `update`).
  Init for the initial scene runs eagerly (matches `gameLoopRun`);
  init for pushed scenes runs at the next VBlank dispatch (full
  budget for DMAs). Static stack of 8 deep, silent no-ops on
  overflow / pop-from-bottom. Closes the framework opt-in trilogy.

- **`examples/basics/scene_stack`** — title → counter → pause overlay
  demo of the scene framework. ~120 LOC, exercises push/pop, eager
  vs deferred init, and shared state via a file-scope global.

### Changed

- **`GameLoopConfig` is now `typedef Scene` (audit H1)** —
  field-for-field identical structs unified. Source-compatible: every
  existing `GameLoopConfig cfg = { .init = ..., .update = ... }`
  keeps working via the typedef; new code is encouraged to use
  `Scene` directly for vocabulary consistency across the trilogy.

- **`scenePush()` no longer calls `init` synchronously (audit H2)** —
  the dispatcher in `sceneRun` now runs init right after
  `WaitForVBlank()` instead of inside the caller's `update` frame.
  This gives init's DMAs (palette, tiles, tilemap) a fresh ~33 K
  cycle VBlank budget. The initial scene's init keeps its eager
  pre-VBlank semantics so `consoleInit` / `setMode` / `setScreenOn`
  ordering still works as in `gameLoopRun`.

- **`examples/graphics/backgrounds/mode1` and
  `examples/graphics/backgrounds/mode1_bg3_priority` migrated to
  the typed `BgAsset` form (audit H3)** — the `BG_LOAD` /
  `GFX_LOAD` statement-form macros are dropped (one flavour, no
  fuzzy choice between two parallel APIs).

### Removed

- **`oamDrawMetasprite` from `<snes/sprite.h>` (chantier P3.2)** —
  was declared as a "legacy simple interface" with zero callers
  in the entire repository. Drops the symbol and the
  `docs/hardware/OAM.md` table mention.

- **`BG_LOAD` / `GFX_LOAD` statement macros from `<snes/asset.h>`
  (audit H3)** — parallel API to the typed `bgLoad` / `gfxLoad`
  functions. Both did the same thing; the macros are gone, the
  typed form is the only form.

- **`lib/source/opensnes_font_2bpp.h` (chantier P3.2)** — orphan
  font header with 1536 bytes of inlined data and unused `FONT_*`
  macros, referenced by nothing. Drops 219 lines of dead code.

- **`examples/maps/dynamic_map` C64 sprite converter dead code** —
  `convertC64Sprite()`, `getPixel()`, `isBitSet()`,
  `sprite_temp[256]`, the `c64_sprite` extern, and the 144-byte
  data block. Unused since the example was ported. Drops 161 LOC.

### Fixed

- **QBE w65816 emit pads DL/DW init data (chantier C.5)** —
  `compiler/qbe/emit.c` `emit_init_data()` now adds `.dsb N, 0`
  after each emitted directive whose written size is smaller than
  `dtype_size[type]`. Without this, `static const Struct { ptr }`
  values had every field after the first pointer at the wrong ROM
  offset (cproc lays out 8-byte stride for `u8 *`, but WLA-DX
  writes 3 bytes for `.dl <symbol>`). The bug had never manifested
  in shipping code because no example carried such a struct; the
  fix unblocks the typed `BgAsset` value shipped in D.2.

- **`scenePush` from `update` no longer overruns the VBlank
  budget (audit H2)** — the pushed scene's init used to share the
  caller's already-consumed window; if init did heavy DMAs it
  could exceed the budget and the PPU would silently drop the
  writes. See "Changed" above for the dispatcher refactor.

### Compiler

- **JSL-in-conditional codegen bug confirmed silently fixed
  (chantier C.7)** — the `MODE_LARGE_SIZE` / `MODE_SMALL_SIZE`
  workaround macros (which existed solely to keep helper calls
  from inlining as JSLs inside an `if (sz == 0) { sz = helper(); }
  if (sz == 8) ...` chain) are converted back to plain functions
  in their own translation unit (`lib/source/sprite_dynamic_helpers.c`).
  `slopemario` (the historical repro) and four other examples
  that link the dynamic sprite engine still render correctly with
  real `jsl` calls in the codepath. Memory note
  `cc65816_conditional_jsl_codegen_bug.md` updated to mark FIXED.

- **`compiler/PINS.md` qbe SHA bumped** to pull in the C.5 fix.

### Documentation

- **Framework trilogy headers cross-reference each other** —
  `<snes/scene.h>` carries an explicit "`Scene` ≡ `GameLoopConfig`"
  paragraph, `<snes/gameloop.h>`'s "When NOT to use this" points
  at `<snes/scene.h>` for state-machine needs, and `asset.h` drops
  the "two flavours" section in favour of "typed value is the
  contract".

- **Stale doc references purged**: `gameloop.h`'s "future C.3 may
  close [the indirect-call TCO] gap" speculative promise dropped,
  the "scene_2d / scene-stack module that's planned next" forward
  reference updated to point at the now-shipped scene module,
  `asset.h`'s "may be added once the patterns settle" sprite-asset
  hint replaced with "out of scope here".

- **`scene.h` documents the contract edge cases** — null `update`
  is documented as undefined (no runtime check by design), the
  `scenePop`-from-`init` pattern pops the scene before its first
  update, push-cascade from `init` runs all chained inits in the
  same VBlank window, scene-swap idiom is `scenePop(); scenePush(next);`,
  inter-scene argument passing uses globals (out of scope by
  design).

- **`docs/mainpage.md` API Reference** lists the three framework
  headers under a new "Framework opt-ins" subsection.

- **README.md and `examples/README.md` example counts refreshed**
  from 53 / 36 to the actual 54.

- **`KNOWN_LIMITATIONS.md` refreshed** — two entries promoted
  🟠 → 🟢 (WLA-DX `.ACCU`/`.INDEX` tracking, now lint-enforced;
  SuperFX/snes9x detection, now covered by the Mesen2-headless
  CI phase). Two stale entries removed (TCO and A-cache through
  `pha` — both fixed in earlier chantiers but still listed as
  open). The "five [KNOWN_BUG] entries" intro line corrected to
  "one" — the only remaining compiler known-bug is the cosmetic
  `leaf_opt=1` comment marker on non-leaf frameless functions.

### Internal cleanup

- **`MODE_LARGE_SIZE` / `MODE_SMALL_SIZE` extracted to a shared
  internal header (chantier P3.2)** — were duplicated identically
  in `sprite_dynamic_dispatch.c` and `sprite_dynamic_meta.c`. Now
  live in `lib/source/sprite_dynamic_internal.h` (and after C.7
  was confirmed fixed, the macros became plain functions in
  `sprite_dynamic_helpers.c`).

- **`sprite_dynamic_dispatch.c` "legacy" comments → "internal"
  (chantier P3.2)** — the ASM entry points behind `oamDynamicInit`
  / `oamDynamicDraw` are not deprecated, they are the
  implementation layer.

### Process / build

- **`.gitignore`: track `lib/source/**/*.h`** — required because
  internal-only headers (`sprite_dynamic_internal.h`) live next to
  their `.c` files, but the previous rule treated all `.h` outside
  `lib/include/` as build artefacts.

## [0.15.2] - 2026-05-01

First of the three "framework opt-ins" promised by `PHILOSOPHY.md`
(alongside the planned scene/state stack and the asset bundle
convention). Ships an opt-in `gameloop` module that owns the
`while (1) WaitForVBlank(); update();` cadence so user code can stay
focused on its own logic.

### Added

- **`<snes/gameloop.h>` + `gameloop` library module (chantier D.1)** —
  opt-in via `LIB_MODULES`, not auto-included by `<snes.h>`. Public
  surface is one struct and one function:
  ```c
  typedef struct {
      void (*init)(void);     // optional, NULL to skip
      void (*update)(void);   // required, MUST NOT be NULL
  } GameLoopConfig;

  void gameLoopRun(const GameLoopConfig *cfg);  // never returns
  ```
  Implementation is three lines of actual logic. The framework owns
  the WaitForVBlank → update cadence and nothing else: no
  `consoleInit()`, no `setMode()`, no screen on/off — all of that
  remains caller-driven so an existing example can opt in by
  extracting init and update into static functions and having
  `main()` hand off to `gameLoopRun`. Documented in the header,
  including the explicit "When NOT to use this" section listing
  state-machine and custom-rhythm patterns that don't fit.

### Changed

- **`examples/basics/timer`, `examples/basics/random`,
  `examples/input/controller`** migrated to the gameloop framework
  as canonical demos of the common pattern (single VBlank sync, an
  `update` that reads input and writes to the tilemap, no custom
  rhythm). ROM bytes change (one indirect call per frame for the
  update dispatch); visual regression at frame 120 is identical for
  controller and timer, shifts ~62 pixels for random's DEC line —
  a one-pixel-column timing offset from the indirect-call overhead
  that's visually indistinguishable. basics/random's baseline was
  refreshed to absorb the shift.

- **`examples/games/breakout` and `examples/games/tetris` deliberately
  NOT migrated.** Both have custom synchronisation rhythms — breakout
  ends each frame with `WaitForVBlank(); oamUpdate();` (work-then-
  sync, with the OAM DMA piggybacking on the same VBlank), tetris
  drives WaitForVBlank from inside per-state functions. The framework
  imposes sync-then-work; migrating either would either drop them to
  30 fps or require restructuring that obscures more than it teaches.
  The "When NOT to use this" section in `gameloop.h` is calibrated
  against exactly these patterns.

### Documentation

- **PHILOSOPHY.md → `gameloop`** — first of the three opt-in framework
  pieces is no longer a placeholder.

## [0.15.1] - 2026-05-01

Audit closure pass. No SDK-consumer-visible changes; the work
finishes off two items from the original 2026-04-26 remediation
plan that the v0.15.0 cycle left partial.

### Added

- **`devtools/lint_asm.py`** — enforces the explicit `.ACCU` /
  `.INDEX` marker policy after every `rep`/`sep` in hand-written
  `.asm` files (lib/source/, templates/). The bug this prevents
  is silent: WLA-DX's per-object-file mode tracking loses
  precision at branch merges and at section boundaries; the
  assembler has historically shipped 2-byte `cpx #0` (8-bit
  immediate) where 3 bytes were needed, cascading misalignment
  through every following instruction. Explicit markers override
  the inferred state — they are pure directives so adding them
  is byte-for-byte identical to the prior ROM. Wired into the
  `Lint` GitHub workflow as the `asm-markers` job.
- **`tools/sa1-patch/`** — dedicated C tool that flips bits 0-1
  of byte $7FD5 in a freshly-linked .sfc to mark it as SA-1
  rather than LoROM. Replaces a python3 one-liner that used to
  live inline in `make/common.mk`, fitting the same source-tree
  pattern as `gfx4snes`, `font2snes`, `smconv`, etc. (audit P2.4
  #3).

### Fixed

- **`devtools/lint_commits.py`** — skip GitHub-style merge commit
  subjects (`Merge pull request #N from owner/branch`,
  `Merge branch 'X' into Y`, `Merge remote-tracking branch ...`).
  Previously every release-PR merge into `main` failed the Lint
  workflow because the auto-generated wrapper subject doesn't
  satisfy Conventional Commits; happened on PR #35 and PR #36
  during the v0.15.0 release flow. The contributor's own commits
  in the same range are still linted individually, and the
  merge commit's body still gets the `Co-Authored-By:` rejection
  check. `.claude/rules/commits.md` updated alongside.

### Build

- **490 `.ACCU` / `.INDEX` markers added** across 21
  hand-written `.asm` files (lib/source/ + templates/),
  generated by `devtools/lint_asm.py --fix`. Top contributors:
  `templates/crt0.asm` (58), `lib/source/sprite_dynamic.asm`
  (69), `lib/source/audio.asm` (55), `lib/source/map.asm` (47),
  `lib/source/snesmod.asm` (46). Verified byte-identical against
  the v0.15.0 ROMs via SHA-256 over all 53 examples.

### Documentation

- **`.claude/rules/testing.md`** gains an "Impacted-Examples
  Triage" section that codifies the workflow used during the
  chantier C TCO validation: identify candidate examples →
  triage to a representative subset → present a small table
  with one "what to look for" per kept entry. Replaces the
  previous "list ALL impacted examples" instruction, which
  produced unreadable walls for Class A changes.
- **`.claude/rules/commits.md`** — new section documenting the
  merge-commit exemption for the lint policy.

## [0.15.0] - 2026-04-30

Audit-driven maintenance cycle (see `~/opensnes_audit_2026-04-26.md` and
`~/opensnes_remediation_plan_2026-04-26.md`). 19 of 23 plan items completed
across P0-P4. The SDK is now end-to-end CI-enforced: build green no longer
just means "it compiles" — it means visual regression, lag detection, runtime,
compiler patterns, bank $00 layout and submodule pins all hold. Adds the
`PHILOSOPHY.md` design-principles document, completes chantiers B (sprite)
and T (text) of P3.2 (public API surface 28 → 18 functions for sprite,
18 → 15 for text), lands the Mesen2-headless visual phase (P3.4) for real
GSU/SA-1 emulation coverage on CI, and ships the chantier C trilogy
(C.1 + C.2.1 + C.2.2) of tail-call optimisations in qbe/w65816 — 18 TCO
sites across the SDK, 4 of the 5 documented known compiler bugs cleared.

### Added

- **`PHILOSOPHY.md`** — design-principles document making the project's
  positioning explicit. OpenSNES is a 2D game engine for C developers
  who don't want to learn 65816 assembly to ship a SNES game; PVSnesLib
  is a thin C wrapper for hardware enthusiasts. The two sit at
  different altitudes of the same stack. Five principles guide every
  API decision (sane defaults with escape hatches, hidden quirks with
  documented escape, opt-in modules, type-safe boundaries, predictable
  performance). Linked from `README.md`, `CLAUDE.md`, `ROADMAP.md`.
- **`KNOWN_LIMITATIONS.md`** — public catalog of silent failures with
  severity tags (🔴 silent corruption / 🟠 silent build / 🟡 toolchain quirk /
  🟢 mitigated). 14 entries covering VBlank, DMA budget, WRAM port, bank $00,
  push order, `volatile` + QBE, `.ACCU` tracking, SA-1 SIWP, SuperFX/Mesen2,
  compiler optimisation gaps, type sizes, sprite-palette CGRAM offset.
- **`compiler/ABI.md`** — empirically verified calling-convention reference:
  LEFT-TO-RIGHT push, frame layout, return values, direct-page layout
  (`tcc__r*`), type sizes, calling C↔ASM templates, port-from-PVSnesLib
  checklist.
- **`compiler/PINS.md`** — pinned SHAs for `compiler/{cproc,qbe,wla-dx}` with
  a per-submodule list of carried-forward local patches.
- **`tools/opensnes-emu/test/BASELINES.md`** — visual-regression baseline
  schema and regen protocol.
- **`lib/contrib/` directory** — non-core engine modules. Currently houses
  `object.asm` (3 124 LOC game-entity engine) relocated from `lib/source/`
  so `lib/source/` only contains hardware-wrapping code.
- **Section "Who is OpenSNES for?"** in README — explicit prerequisites
  (65816 ASM, NMI/VBlank model, hex addresses) and non-targets, plus an
  enhancement-chip maturity table.
- **Branching policy** in `CONTRIBUTING.md` — invariants for `main` vs
  `develop`.
- **2 compiler regression guards** — `test_arg_push_order` (LEFT-TO-RIGHT
  ABI) and `test_section_directives` (`.ACCU 16` / `.INDEX 16` markers).
  Compiler test suite: 60 → 62.

### Changed

- **`tools/opensnes-emu/` is now a public submodule** at
  `github.com/k0b3n4irb/opensnes-emu` (was a local-only directory in the
  user's tree). History was rewritten with `git filter-repo` to strip 275
  generated build artifacts (.sfc, .sym, .o, .obj, etc.) — repo size went
  from 218 MB to 11 MB tracked source.
- **CI runs the full ~390-check test suite** (was build-only). Drop of
  `--quick` adds the 60-test compiler-pattern phase. Added
  `--allow-known-bugs` so the 5 documented compiler-optimisation gaps
  (TCO + A-cache-through-pha + stale `leaf_opt` marker) report as
  `[KNOWN_BUG]` rather than failing the build.
- **`ROADMAP.md` resync** — corrects 52 → 53 examples, 212 → ~390 checks,
  60 → 62 compiler tests, 7 → 8 phases. Drops removed modules
  (animation, entity). Adds the contrib status for `object`. Pivots to
  the "post-v0.13.0 toward v0.14.0" snapshot.
- **`.claude/` is now tracked** (was entirely gitignored except one file).
  Hooks, rules, skills and shared settings ship with the repo so any
  Claude Code user gets the same gates. Personal `settings.local.json`
  remains gitignored.
- **`setMainScreen()` instead of `REG_TM = ...`** in 22 examples (style
  consistency; identical bytes generated). `short` → `s16` in three more.
- **Sprite API simplification (P3.2 chantier B)** — public sprite surface
  collapses from 28 to 18 functions:
  - `oamSet`/`oamSetEx`/`oamInit*` consolidations (B.A);
  - `oamDynamicDraw(id)` size-aware dispatcher replaces
    `oamDynamic{8,16,32}Draw` (B.1+B.2);
  - `OamDynamicConfig` struct + `oamDynamicInit(&cfg)` replace the
    5-arg positional `oamInitDynamicSprite` (B.3);
  - migrated 9 call sites across 5 example games + reverted a
    Y-1 over-compensation in the dynamic engine that was lifting
    Mario 1–2 px off the ground on flat terrain (B.4);
  - the NMI handler auto-flushes the dynamic engine via a new
    `dynamic_flush_hook` indirect call — main loops drop the
    explicit `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    pair (B.5);
  - legacy `oamInitDynamicSprite` and `oamDynamic{8,16,32}Draw`
    retire from the public header; their ASM symbols stay as the
    internal mechanism (B.6 modest);
  - `oamMetaDrawDyn(id, x, y, meta, gfx, OBJ_SMALL/OBJ_LARGE)`
    replaces `oamMetaDrawDyn{8,16,32}` — engine resolves pixel size
    from the size pair set at init (B.6 aggressive part 1);
  - simple init flushes (likemario, slopemario, dynamic_sprite)
    migrate to a single `WaitForVBlank()` that fires the auto-flush
    hook under force blank (B.6 aggressive part 2 partial);
  - `oamDynamicDrainQueue()` lets the multi-VBlank init-time drain
    happen via the same NMI auto-flush, with a draining flag that
    inhibits the end-frame hide step during the drain — which lets
    the legacy `oamInitDynamicSpriteEndFrame` / `oamVramQueueUpdate`
    retire from the public header for good (B.6 aggressive part 2
    complete).
- **Text API simplification (P3.2 chantier T)** — public text surface
  collapses from 18 to 15 functions:
  - `textPrintS16`, `textDrawBox` retired; internal `textFillRect`
    made `static` (T.1) — none had callers across the 53 examples
    or the library;
  - text writers (`textPutChar`, `textClear`, `textFillRect`)
    auto-flush via the NMI tilemap_update_flag (T.4) — 17 examples
    drop the manual `textFlush()` call. `textFlush()` stays public
    as the explicit primitive for hand-rolled tilemap writers.
- **`textGetX()` / `textGetY()` restored** — initially dropped in
  T.2 on a "zero callers" sweep, then reintroduced after the unit
  test fixture in opensnes-emu surfaced as a legitimate caller
  reading cursor advancement. Two 1-line accessors are the right
  shape for testable internal state.
- **NMI handler gains a `dynamic_flush_hook` 24-bit function pointer**
  in the bank-$00 register area. Defaults to a single-`rtl` no-op
  stub; `oamInitDynamicSprite` repoints it at `oamDynamicNmiFlush`
  which calls `oamInitDynamicSpriteEndFrame` + `oamVramQueueUpdate`
  every VBlank. Cost for ROMs that don't use the dynamic engine: one
  PEA + JML indirect + RTL ≈ 25 cycles per frame.
- **Mesen2-headless visual regression phase (P3.4)** — second visual
  phase that runs the four chip-using ROMs (SuperFX 3D, SuperFX
  Hello, SA-1 Hello, SA-1 Starfield) through Mesen2 in `--testrunner`
  mode for real GSU/SA-1 emulation. snes9x's libretro core in
  opensnes-emu does not detect the GSU chip in our ROM headers, so
  its visual phase only validates "boots without crashing" for those
  ROMs; Mesen2 fills the gap. Vendored `vendor/Mesen` binary +
  `scripts/install-mesen2.sh` for fresh contributor setup.
- **TCO for trivial wrappers (chantier C.1)** — qbe/w65816 backend
  now emits `jml callee` instead of the full `jsl + frame teardown
  + rtl` for non-leaf functions whose every Ocall is in tail
  position (e.g. `unsigned short call_add(a, b) { return add_u16(a, b); }`).
  The TCO emission path was already in place; the remaining gate
  was the conservative "non-leaf needs a frame" rule, relaxed to
  recognise that a function with only tail Ocalls never executes
  past a call and so cannot have temps that need across-call
  storage. Result: 3 of the 5 known compiler-bug tests cleared
  (plx_cleanup wrapper variants, acache_pha through pha, lazy_rep20
  for pure tail calls); −5 cycles + −2 ROM bytes per tail call site.
  Includes an env-gated diagnostic (`CC_TRACE_TCO=1`) that logs each
  TCO eligibility decision — zero-cost when the env var is unset.
- **TCO for framed void-tail-call wrappers (chantier C.2.1)** —
  extends C.1 to functions that have a real frame because of an
  earlier non-tail call, but whose final tail call takes zero
  arguments. The Ocall handler now emits a `tsa; clc; adc.w
  #framesize; tas` teardown immediately before the `jml` (gated on
  `framesize > 2` to mirror the prologue's elision of phantom
  alignment-only frames). 10 new TCO sites unlocked across the SDK,
  including `oamDynamicDrainQueue`, `textInit`, `run_frame` (breakout
  main loop), `stateGameOver`/`stateTitle` (tetris), `changeObjSize`
  (object_size example) and `koopatroopaupdate` (likemario AI).
- **TCO for chained tail calls (chantier C.2.2)** — closes the
  `return f(g(x))` pattern, where the inner call is a regular jsl
  (its result feeds the outer arg) and the outer is a tail call.
  The Oarg handler stores each outgoing arg to its caller-slot
  position (offset `4 + framesize + ...` while the frame is still
  up); the existing Ocall teardown then lifts SP back to the
  function-entry point so the callee sees the args at canonical
  `4`, `6`, ... offsets. Required two supporting changes:
  count_fn_param_bytes() now falls through to a slot-walking helper
  for non-leaf functions where Opar* has been lowered to slot
  reads, and an alloc-safety guard declines C.2.2 on functions
  containing any local Oalloc* (the frame teardown would invalidate
  any pointer-to-local that escaped via the tail call — caught
  the hard way: textPrintU16's `char buf[6]` + `textPrint(p)`
  pattern produced a 101-pixel diff in basics/random's DEC line
  before the guard landed). Cleared the last TCO known-bug
  (`tail_call`'s call_chain check) and unlocked 3 more sites in
  the lib (mouseGetX/Y, colorMathTransparency50). Test suite
  without `--allow-known-bugs`: 397/399 → 398/399; the only
  remaining known-bug is nonleaf_frameless's stale ASM-comment
  marker, orthogonal to TCO.

### Fixed

- **SNES PPU sprite Y +1 scanline quirk** — caller-passed Y was off
  by one row from the rendered top because OAM_Y=N renders on
  scanlines N+1..N+8. The `oamSet` ASM and `oamSetY` C path applied
  the `-1` compensation in 87a0ae2; the macros `oamSetFast` /
  `oamSetXYFast` were missed and silently rendered one scanline
  lower than the equivalent `oamSet` call. Now uniform across all
  five entry points so callers can mix the fast and slow APIs without
  drift. `oamHide` is exempt (writes 240 directly).
- **`lib/source/hdma.c:89`** — `sine_quarter[63] = 256` truncated to 0 in
  `u8`, leaving a 1-pixel notch at sin(90°) on every HDMA sine effect.
  Cap the table at 255.
- **`examples/games/tetris/render.c:346`** — `u8 tile` couldn't hold
  `TILE_BORDER_H | PAL1 = 0x409`, dropping the palette bits silently and
  rendering the line-clear flash with the wrong palette. Promote to `u16`.
- **`examples/memory/sa1_starfield/`** — `USE_FASTROM := 1` eliminates
  the 50 % VBlank lag (150/300 frames → 0/300). The 128-bird OAM-update
  loop was running into the next NMI on SlowROM.
- **Hooks `pre-commit-check.sh` / `mark-tests-passed.sh`** — outputted
  `{"decision": "allow"}` (invalid for the Claude Code schema) and
  pointed contributors to non-existent test scripts. Now run silently
  on the pass path and reference the real
  `node test/run-all-tests.mjs --quick` runner.
- **`.gitignore` cleanup** — `.claude/` entry was a blanket gitignore that
  hid project-wide policies. Replaced with the precise
  `.claude/settings.local.json` exclusion.
- **3 hardcoded `/home/kobenairb/...` paths** in `.claude/rules/` and
  skills replaced with `$PVSNESLIB_HOME`.
- **Two dead unit fixtures** (`unit/animation`, `unit/entity`) referenced
  modules removed from `lib/`. Build phase passed from 76/78 to 76/76.
- **2 stale visual baselines** (`graphics/sprites/dynamic_metasprite`,
  `maps/slopemario`) regenerated after recent ASM/c-bit fixes; baseline
  for `memory/sa1_starfield` captured for the first time after the
  FastROM bump.
- **12 latent C warnings** surfaced by the new clang lint pipeline —
  unused locals, dead helper functions (`writestring_bg2`, `refresh`),
  unused parameters in callback ABI signatures.

### Build

- **Bank $00 ROM overflow check** runs on every link
  (`devtools/symmap/symmap.py --check-bank0-overflow` invoked from
  `make/common.mk`). String-literal spills now fail the build instead of
  producing garbage at runtime. `SKIP_BANK0_CHECK=1` escapes if needed.
- **`make verify-toolchain`** compares each compiler-submodule HEAD
  against `compiler/PINS.md`. CI calls it before every build — drift
  fails fast with a fix-it message naming the right remediation.
- **clang `-fsyntax-only -Wall -Wextra -Werror`** runs in parallel with
  cc65816 in the `%.c.o` rule (cproc ignores `-W*`). The SDK is
  warning-clean and stays that way. `SKIP_LINT=1` for environments
  without clang.
- **Sed QBE→WLA-DX transform removed.** Empirical audit showed 9 of 10
  patterns matched nothing in current QBE output (QBE emits `.db`/`.dw`/
  `.dl` natively). The 10th pattern only deleted `/* end */` comments
  that WLA-DX accepts. `lib/Makefile` lost 12 lines of fragile sed.
- **Memmap dependency tracking** — `wrap_asm` consumers now list
  `$(MEMMAP_INC)` as a real prerequisite. `touch templates/memmap.inc`
  triggers a rebuild instead of producing stale objects.
- **FastROM Makefile flag** documented; SA-1 Starfield uses it.

### CI

- **Functional-tests job** — opensnes-emu test suite gates PR merges
  on Linux. Caches WASM core build by `bridge.cpp` hash; cache hit
  brings full-suite runtime to ~3 minutes.
- **Tag-on-main guard** — `release.yml` rejects a tag whose commit is
  not reachable from `origin/main`. Tags can only be cut from the
  release-PR merge.
- **Lint workflow** (`lint.yml`) — runs `lint_commits.py` on every PR
  and direct push to `main`/`develop`.
- **Lint workflow handles force-push** — after a force-push,
  `github.event.before` points to a now-dangling commit that
  `actions/checkout` does not fetch, so `git log ${BEFORE}..HEAD`
  used to fail with exit code 2 (indistinguishable from a real lint
  violation in the workflow output). Now `git rev-parse --verify`
  the SHA first and fall back to `origin/main..HEAD` if it does
  not resolve. The lint policy itself is unchanged.
- **Mesen2-headless phase wired up on CI** — runs on `ubuntu-22.04`
  (matches Mesen2's published x64-AOT build environment, avoids the
  libstdc++ ABI mismatch that surfaces on 24.04 as `std::bad_cast`
  in `MesenCore.so::InitDll`). Installs `xvfb` for a virtual display
  and `libsdl2-dev` for the runtime SDL2 the native core dlopens at
  startup. A 128 KB sanitised settings.json fixture in
  `tools/opensnes-emu/test/fixtures/mesen2-config/` is copied to
  `~/.config/Mesen2/` before launch — bypasses the first-run wizard
  and provides the per-system config subtrees `MesenCore.so`
  dynamic_cast<>s during init.
- **`visual-mesen2.mjs` auto-wraps Mesen2 in `xvfb-run -a`** when
  `$DISPLAY` is unset, so the same phase runs on a developer's
  desktop and on a headless CI runner without conditional logic.
  Failure messages now include the trailing 2 KB of Mesen2 stdout
  / stderr, surfacing the actual failure signature (DllNotFound,
  std::bad_cast, etc.) inline in the CI log instead of as an opaque
  exit code.

### Testing

- **Visual-regression baselines** carry provenance metadata
  (`rom_sha256`, `snes9x_commit`, `captured_at`). The runner emits
  `[WARN]` on drift instead of silently producing a misleading diff.
- All 53 baselines re-captured against the current `make`-built ROMs.
- **Compiler test runner** marks 5 unimplemented optimisations as
  `knownBug()` instead of `fail()` — TCO (3 tests), A-cache through
  pha (1), and the stale `leaf_opt=1` marker for non-leaf functions
  (1). Without `--allow-known-bugs` they still fail; with the flag
  they report `[KNOWN_BUG]` so a contributor implementing one sees
  green immediately.
- **Mesen2 visual baselines** added for the four chip-using ROMs
  (`graphics/effects/superfx_3d`, `memory/superfx_hello`,
  `memory/sa1_hello`, `memory/sa1_starfield`) under
  `tools/opensnes-emu/test/baselines/*.mesen2.bin`. Per-ROM diff
  overrides for the two animated ones (cube rotation 2000 px,
  starfield scroll 1500 px) absorb 1-frame timing drift between
  machines without losing the "chip ran vs chip-NOT-DETECTED black
  screen" signal that is the actual point of the phase.
- **Unit fixture sync after sprite Y quirk** — 13 Y assertions in
  the opensnes-emu `unit/sprite` fixture updated to expect the
  `(input - 1)` convention now applied uniformly across the OAM
  setters. Header note documents the convention so future test
  authors see it. Phase result: `73/73 passed` (was 14 fail / 59
  pass before the sync).

### Tooling

- **`devtools/verify_toolchain.py`** — parses `compiler/PINS.md` and
  enforces submodule-pin invariants.
- **`devtools/lint_commits.py`** — Conventional-Commits subject
  validation + `Co-Authored-By:` rejection.

### Documentation

- **`compiler/ABI.md`**, **`KNOWN_LIMITATIONS.md`**,
  **`tools/opensnes-emu/test/BASELINES.md`**, **`lib/contrib/README.md`**
  added as canonical references.
- **`README.md`** gains the audience-explicit section, chip maturity
  table, and a prominent link to `KNOWN_LIMITATIONS.md`.
- **`ROADMAP.md`** resynchronised against current state.
- **`CONTRIBUTING.md`** documents the branching policy.
- **`.claude/rules/release.md`** has the full release flow with ASCII
  diagram and `make verify-toolchain` in the pre-release checklist.
- **5 stale "212 checks" / "52 examples"** references corrected across
  `.claude/rules/*.md` and the README.

## [0.13.0] - 2026-03-26

### SuperFX Phase 3-4 — Mandelbrot + Wireframe Cube

- **FMULT validated**: 4.12 fixed-point multiply confirmed (2.0×2.0=4.0, 1.5×3.0=4.5).
  Formula: `result_4_12 = FMULT_output << 4` (4× ADD R0).
- **Mandelbrot fractal**: 256×128 computed by GSU via FMULT+PLOT, 16-color palette,
  4.12 fixed-point iteration (z=z²+c, 15 max iterations). LOOP instruction for
  Y iteration (body exceeds BNE 8-bit range). CACHE for ~3× speedup.
- **Wireframe cube**: 12-edge Bresenham line drawing via PLOT, Y+X axis rotation
  computed in C (sin/cos table), orthographic projection. Chunked 4×4KB VBlank DMA
  (no flicker, ~15 FPS). Function splitting to avoid framesize>255.
- **WRAM stub overflow fix**: stub grew to 90 bytes with parameterized SCBR/R8,
  gsu_wram_area increased from 64 to 96 bytes.
- **Mesen2 backward branch bug**: confirmed via bsnes comparison. BNE/BRA+STW
  works on bsnes (cycle-accurate) but corrupts on Mesen2. bsnes is the reference
  emulator for SuperFX testing.

### Documentation

- **READMEs + screenshots**: all 4 SuperFX examples fully documented with
  emulator compatibility table (bsnes/Mesen2/snes9x).
- **EXAMPLES_BY_CATEGORY.md**: added Enhancement Chips section (SA-1 + SuperFX).
- **LEARNING_PATH.md**: added Level 6 (Enhancement Chips).
- **REGISTERS.md**: added SA-1 ($2200-$230E) and GSU ($3000-$303F) register tables.
- **MEMORY_MAP.md**: added SA-1 I-RAM/BW-RAM and SuperFX SRAM/cache layouts.

### Examples (56 total, +4 new SuperFX)

- **superfx_hello**: boot diagnostic + SRAM byte/word + FMULT 4.12 validation
- **superfx_bitmap**: 16-color gradient via PLOT hardware
- **superfx_mandelbrot**: fractal set via FMULT + PLOT (4.12 fixed-point)
- **superfx_3d**: rotating wireframe cube (Bresenham + PLOT, 2-axis rotation)

## [0.12.0] - 2026-03-23

### SuperFX (GSU) Enhancement Chip Support (NEW)

- **SuperFX coprocessor**: full build system support (`USE_SUPERFX=1`).
  Two-stage GSU assembly pipeline: `.sfx` -> `wla-superfx` -> `.sfx.bin` -> `.incbin`.
- **PLOT bitmap rendering**: GSU renders 16-color gradient via hardware PLOT
  instruction at 21.47 MHz with CACHE optimization. Column-major tile layout,
  pixel cache flush via RPIX.
- **Three critical GSU rules discovered**:
  1. Branch delay slot -- NOP after every BNE/BRA (instruction always executes)
  2. STOP pre-fetch -- NOP padding before STOP (pipeline halts prematurely)
  3. RPIX pixel cache flush -- last 8 pixels per row stay in internal cache
- **superfx_hello**: boot diagnostic with STB/STW SRAM write tests
- **superfx_bitmap**: 16-color rainbow gradient rendered by GSU PLOT hardware
- **superfx.h**: complete GSU register definitions ($3000-$303F)
- **WRAM execution**: mandatory for all GSU launches (ROM bus exclusive)
- **21.47 MHz + CACHE**: ~6x speedup over uncached 10.74 MHz baseline

### SA-1 Enhancement Chip

- **SA-1 murmuration demo**: 128 dots in Lissajous sine patterns at 10.74 MHz.
  Uses `lda.l sine_table,x` (opcode $BF) for DB-independent ROM reads.
  4 brightness palettes for depth illusion on dark blue background.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  single source of truth in `templates/memmap*.inc`.
- **runtime.asm moved to lib/**: compiled once per config instead of per-example.
- **Per-example SA-1/SuperFX boot**: `project_sa1_boot.asm` generated from
  local override or template default.

### Documentation

- **SuperFX tutorial**: `docs/tutorials/superfx.md` covering architecture,
  PLOT rendering, assembly rules, WRAM execution, emulator compatibility.
- **SA-1 tutorial**: `docs/tutorials/sa1.md` with I-RAM patterns and debugging.
- **GETTING_STARTED.md rewritten**: Game Developer vs SDK Developer paths.
- **All examples documented**: 54 READMEs + screenshots (opensnes-emu).
- **Documentation audit**: fixed broken links, updated example count, added
  tutorials index.

## [0.11.0] - 2026-03-21

### SA-1 Enhancement Chip Support (NEW)

- **SA-1 coprocessor**: full build system support for SA-1 cartridges
  (`USE_SA1 := 1`). Includes ROM header (cart type $35), memory maps,
  post-link $FFD5 patching, and per-example boot stub override.
- **SA-1 boot infrastructure**: crt0.asm initializes SA-1 (reset vector,
  I-RAM write protection SIWP=$FF, release from reset, magic byte handshake).
- **Per-example SA-1 boot**: each SA-1 example provides its own `sa1_boot.asm`.
  crt0.asm includes `project_sa1_boot.asm` (local override or template default).
- **sa1.h / sa1.c**: register definitions ($2200-$230E), `sa1Init()` function.
- **Key discovery**: SIWP/CIWP bit=1 means WRITABLE (not protected despite
  the register name). Documented in `.claude/SA-1.md`.

### Build System

- **Unified memmap files**: deleted 3 duplicate `lib/source/lib_memmap*.inc`,
  all 18 library ASM files now reference `templates/memmap*.inc` via
  `-I ../templates`. Single source of truth for memory maps.
- **runtime.asm moved to lib/**: compiled once per config (lorom/hirom/sa1)
  instead of 46 times per example. `RUNTIME_OBJ` always linked from library.

### Compiler

- **WLA-DX .ACCU/.INDEX override warning**: detects when `rep`/`sep` tracking
  diverges from explicit `.ACCU`/`.INDEX` directives after branch merges.
  Flags reset at `.ENDS` boundaries to avoid false positives.
- **Leaf optimization fix**: `leaf_opt = fn->leaf && !fn->dynalloc` (was
  `!fn->dynalloc`). Prevents non-leaf functions from corrupting JSL return
  addresses with SSA temporaries.

### Runtime

- **Dynamic sprite state moved to bank $00**: RAMSECTION relocated from
  bank $7E slot 2 to bank 0 slot 1 — C code can now access variables
  via `lda.l $xxxx` (WRAM mirror, below $2000).

### Examples (52 total, +3 new)

- **sa1_hello**: SA-1 boot diagnostic — displays coprocessor status codes.
- **sa1_speed**: SA-1 32-bit counter at 10.74 MHz — shows 69K increments/frame.
- **sa1_starfield**: 128-dot murmuration with Lissajous sine patterns.
  4 sine lookups per bird using `lda.l sine_table,x` (DB-independent ROM reads),
  4 brightness palettes for depth illusion on dark blue background.

### Documentation

- **SA-1 tutorial**: `docs/tutorials/sa1.md` — architecture, setup, I-RAM
  communication patterns, assembly tips, Mesen2 debugging guide.
- **GETTING_STARTED.md rewritten**: two clear paths — "Game Developer" (download
  release, just needs `make`) vs "SDK Developer" (clone, build from source).
- **6 missing READMEs added**: all 52 examples now have README.md + screenshots
  (generated via opensnes-emu headless API).
- **Documentation audit**: fixed 4 broken links, updated example count 41→52,
  added SA-1 to all doc indexes, added tutorials table to docs/README.md.

## [0.10.0] - 2026-03-21

### Library

- **Dynamic metasprite engine**: `oamMetaDrawDyn32()`, `oamMetaDrawDyn16()`,
  `oamMetaDrawDyn8()` — multi-tile sprite characters with per-frame VRAM streaming.
  Iterates MetaspriteItem arrays and calls the proven oamDynamic*Draw functions
  per sub-sprite. oamMetaDrawDyn16 includes `sprsize` parameter for correct
  name table bit handling in SMALL mode.

### Build System

- **Eliminated `combined.asm`**: each ASM source (crt0, runtime, data_init_start,
  user ASMSRC) is now compiled as a separate object file. Fixes HiROM ROMBANKMAP
  linker errors and removes all `cat >>` file concatenation.
- **common.mk refactoring**: 529 → 303 lines (-43%), 32 → 15 conditionals (-53%).
  New `wrap_asm` macro factors the duplicated memmap-include-and-assemble pattern.
  `_HAS_SOUNDBANK` computed once replaces 6 nested conditional blocks. `$(if)`
  one-liners replace multi-line ifeq/else/endif blocks.
- **smconv patched**: generates FORCE sections with `.ORG 0` and `SOUNDBANK_BANK`
  in header directly — eliminates 3 sed post-processing calls.
- **Soundbank multi-bank support**: soundbanks >32KB correctly split across
  multiple ROM banks with FORCE placement. Tested with 56KB (LoROM, 2 banks)
  and 59KB (HiROM, 2 banks) soundbanks.
- **HiROM soundbank support**: soundbank assembled as separate object to avoid
  ROMBANKMAP conflicts. HiROM `.ORG $8000` override for bank mirror access.
- **HiROM text module fix**: `text.asm` and `text4bpp.asm` now include the correct
  HiROM memmap conditionally (was hardcoded to LoROM, blocking all HiROM+text builds).
- **HiROM ROMBANKS**: increased from 4 to 8 (512KB) to accommodate large soundbanks.
- **macOS compatibility**: `sed -i.bak` instead of `sed -i` for BSD sed portability.
- **Header template**: ROM_NAME via single sed, numeric values (CARTRIDGETYPE,
  ROMSIZE, SRAMSIZE) via `.DEFINE` in project_config.inc.
- **Fixed `make clean`**: removed stale `tests/` directory reference.

### Compiler

- **Zero warnings**: 72 Clang warnings fixed across cproc (67) and QBE (5).
  Strict prototypes, switch default cases, operator precedence parentheses,
  assignment-as-condition, sign comparison, unused variables.

### Tools

- **Zero warnings**: gfx4snes, smconv, img2snes, tmx2snes — strict prototypes,
  uninitialized variables, const qualifiers, missing newlines, duplicate
  instrument name handling in smconv (appends `_N` suffix).

### Examples (49 total, +3 new)

- **dynamic_metasprite**: port of PVSnesLib DynamicEngineMetaSprite — 3 OBJSEL
  configurations (8/16, 8/32, 16/32) selectable via D-PAD, glitch-free transitions.
- **snesmod_music_large**: "What Is Love" 108KB IT module — validates multi-bank
  LoROM soundbank (56KB across 2 banks).
- **snesmod_music_hirom**: "What Is Love" 210KB IT module — validates HiROM
  soundbank support with 64KB banks.
- **likemario**: renamed ACT_STAND/WALK/JUMP/FALL to MARIO_ACT_* to avoid
  conflict with map.h bitmask definitions.

## [0.8.0] - 2026-03-15

### opensnes-emu — Debug Emulator (NEW)

- **SNES debug emulator** powered by snes9x (WASM): load ROMs, run frames, capture
  screenshots, inspect VRAM/CGRAM/OAM/CPU/PPU state programmatically.
- **MCP server** with 14 tools for Claude Code integration (stdio transport).
- **Single source of truth**: `tests/` directory removed. All testing handled by
  `node tools/opensnes-emu/test/run-all-tests.mjs` — 212 checks across 7 phases.
- **Visual regression**: pixel-exact screenshot baselines for all 41 examples.
- **Lag frame detection**: measures steady-state lag over 300 frames per example.
- **Compiler benchmark**: cycle count comparison via `run-benchmark.mjs`.

### Compiler

- **fixMul/fixLerp**: rewritten in assembly using SNES hardware multiplier ($4202/$4203).
  C version overflowed because `(s32)a*(s32)b` compiled to 16-bit `__mul16`.
- **Alloc TSA**: alloc instruction now computes absolute stack address via `TSA+offset`,
  preventing aliasing with tcc compiler registers at $0000-$001F.
- **Benchmark baseline**: 33 functions, 1318 total cycles.

### Library

- **BG_4COLORS0 palette banking**: `bgInitTileSet` now computes CGRAM offset as
  `bgNumber*32 + paletteEntry*4` for Mode 0 (matches PVSnesLib).
- **HDMA channel 7 warning**: NMI handler uses DMA channel 7 for OAM — documented
  in hdma.h, window/transparent_window examples fixed to use channels 4+5.
- **fixMul/fixLerp/fixDiv**: hardware multiplier assembly (lib/source/math.asm).
- **objUpdateXY**: header corrected — parameter is raw index, not handle.
- **MultiPlayer5**: new mp5 module for multitap adapter support.

### Runtime

- **NMI handler**: MP5/mouse/scope extracted to SUPERFREE sections. Auto-joypad wait
  before all input reading. OAM DMA revert (preconfigured channel optimization caused
  Mesen2 regression — snes9x didn't catch it).

### Examples (41 total, +4 new)

- **Mode 0**: 4-layer 2bpp Kirby parallax demo (ported from PVSnesLib).
- **Mode 3**: 256-color 8bpp static display with split DMA loader.
- **Mode 5**: Hi-res 512×256 16-color display.
- **Tetris**: Korobeiniki music (SNESMOD), multi-line clear fix, RNG fix, gravity fix.
- **All 41 examples** now have README.md + screenshot.png.

### Documentation

- Streamlined root README (328→150 lines).
- Removed MATURITY_REVIEW.md (replaced by opensnes-emu real metrics).
- Updated all docs for 41 examples and opensnes-emu workflow.

### Removed

- `tests/` directory (17,701 lines) — fully migrated to opensnes-emu.
- `devtools/benchmark/` and `devtools/check_mvn/` — handled by opensnes-emu.
- `examples/benchmark/` — opensnes-emu handles benchmarking.

## [0.7.1] - 2026-03-10

### Build System

- **WLA-DX submodule updated** to latest upstream (42 commits) — assembler/linker
  improvements and bug fixes.
- **Fixed ASCIITABLE warnings**: moved `.ASCIITABLE` definition from `crt0.asm` into
  all memmap include files so every assembly unit gets the identity ASCII mapping.
- **Added `.ACCU 16` / `.INDEX 16`** to `runtime.asm` for correct WLA-DX register
  width tracking in math functions.
- **Release asset filenames now include version tag**
  (e.g. `opensnes_v0.7.1_linux_x86_64.zip`).

### Tooling

- **VS Code project configuration**: shared `settings.json` (file associations,
  IntelliSense), `tasks.json` (12 build/test tasks), `extensions.json`
  (WLA-DX syntax + C/C++ recommended).

### Dependencies

- **Updated vendored lodepng** from 20230410 to 20260119 in gfx4snes and img2snes.

### Housekeeping

- Added `.gitattributes` to fix GitHub language detection (all `.h` → C).
- Removed accidentally tracked `CLAUDE.md` files from repository.
- Updated `ATTRIBUTION.md` with complete dependency and contributor credits.

## [0.7.0] - 2026-03-10

### CI/CD

- **Pre-built binary releases**: new `release.yml` workflow triggered on version tags.
  Builds SDK on Linux, macOS, and Windows, runs tests, creates GitHub Release with
  platform zips attached automatically.
- Release zip filenames now include architecture (`opensnes_linux_x86_64.zip`,
  `opensnes_darwin_arm64.zip`, `opensnes_windows_x86_64.zip`).
- Build workflow skips tag pushes (handled by release workflow).

### Documentation

- **opensnes-emu debug emulator (single source of truth for testing)
  of OpenSNES vs PVSnesLib across 8 dimensions.
- **Published benchmark** (`docs/BENCHMARK.md`): 34-function comparison showing
  -30.3% total cycles vs PVSnesLib + 816-opt (32/34 function wins).
- Updated ROADMAP from v0.3.0 to v0.6.0 with structured v1.0 milestones.
- Updated README: beta status badge, comparison table, accurate counts (37 examples,
  28 headers, 60 compiler tests, 25 unit modules).
- Getting started guide now offers pre-built SDK download as primary option.

## [0.6.0] - 2026-03-09

### Compiler

- **Signed division and modulo**: emit `__sdiv16` / `__smod16` for signed `/` and `%`
  operators.
- **Fixed cproc `mktype()` uninitialized fields**: garbage in `type->qual` could cause
  mutable structs to be emitted as `.rodata` (ROM). Fixed by initializing all fields.
- Eliminated redundant loads in phi-move A-cache.

### Library

- **HDMA effect helpers**: new high-level library functions for wave, ripple, iris wipe,
  brightness gradient (`hdmaWaveEffect`, `hdmaBrightnessGradient`, etc.).
- Fixed `oamDrawMeta` return value and metasprite mode switching.
- Migrated color gradient HDMA to bank $00 RAM.
- Double-buffer ripple mode fix and edge wrapping.
- Iris tables moved to bank $00 for correct HDMA bank byte.

### Runtime

- Signed 16-bit division and modulo (`sdiv16` / `smod16`) in `runtime.asm`.
- Division-by-zero guard and X register preservation in software division.

### Examples

- HDMA helpers demo example.
- Migrated hdma_wave and hdma_gradient to library helpers.
- Fixed continuous_scroll: replaced fragile nmiSetBank callback with bgSetScroll.

### Build System

- Flattened `templates/common/` to `templates/`.
- Removed dead startup code from crt0.asm and libsnes.asm.

## [0.5.0] - 2026-03-08

### Compiler

- Fixed memory leaks in cproc (cleanup functions added).

## [0.4.0] - 2026-03-07

### Toolchain

- **smconv rewritten in pure C** (was C++). Simpler build, no C++ dependency.

### CI/CD

- cproc segfault retry for Windows MSYS2 stability.
- Replaced cproc|qbe pipe with temp file to catch cproc crashes.
- GitHub Pages deployment workflow for Doxygen docs.

### Documentation

- Restructured documentation with learning path and navigation hub.
- Removed Co-Authored-By requirement from commit messages.

### API

- **Removed 4 deprecated functions**: `padUpdate`, `dmaCopyToVRAM`, `dmaCopyToCGRAM`,
  `dmaCopyToOAM`. Use `padHeld`/`padPressed` and `dmaCopyVram`/`dmaCopyCGram`/`dmaCopyOam`.

## [0.3.0] - 2026-03-05

### Examples

- **Transparency rewrite**: Replaced placeholder color math demo with proper PVSnesLib-style
  transparency example — landscape (BG1, 4bpp) blended with scrolling clouds (BG3, 2bpp) via
  color addition, using assembly DMA loader for correct bank bytes.
- **HDMA gradient fix**: Improved step formula for smoother fade and deferred HDMA activation
  to button press.
- **Asset pipeline modernization**: All 36 examples now use `res/` subdirectories with gfx4snes
  Makefile rules for reproducible asset conversion from source PNGs. Removed tracked generated
  files (`.inc`, `_data.as`, `.pic`, `.pal`, `.map`) — these are now built at compile time.
- Converted all remaining BMP source assets to PNG.
- Added missing `.inc`/`_data.as`/`_meta.inc` entries to clean rules.

### Documentation

- Comprehensive README rewrite for all 36 examples with consistent formatting, hardware
  explanations, and build instructions.
- Added Mesen2 screenshots for key examples (breakout, likemario, collision_demo, etc.).

### Devtools

- Reorganized `devtools/` into one directory per tool, each with its own README.

### Housekeeping

- Removed unused project templates.
- Removed tracked gfx4snes-generated binary files from the repository.
- Fixed sprite32.pal size and removed unused assets.

## [0.2.0] - 2026-03-03

### Compiler

- **Fixed variable shift codegen** — `(1 << variable)` generated broken code because WLA-DX
  defaulted to 8-bit index registers per object file. Fixed by emitting `.ACCU 16` / `.INDEX 16`
  before each function.
- **Fixed variable shift stack offset** — `emitload_adj` now correctly adjusts +2 after `pha`
  in the shift loop.
- **-22% runtime cycles** vs PVSnesLib + 816-opt on benchmark suite (improved from -31% in v0.1.0)
- Added composite constant multiply (*20, *24, *40, *48, *96)
- Added inline multiply for *11 through *15
- Dead store elimination for inline multiply A-cache compatibility

### Library

- New modules: `map`, `object`, `debug`, `lzss`, `video`
- Mouse and Super Scope input drivers
- Assembly `oamSet()` — C version had framesize=158, causing visible slowdown with >2 sprites
- WAI-based `WaitForVBlank()` — saves CPU power
- Conditional BG scroll writes via `bg_scroll_dirty` bitmask in NMI handler
- NMI callback skip optimization (~260 cycles saved when using DefaultNmiCallback)
- `oamSetFast` macro for zero-overhead OAM writes
- Fixed `mode7Rotate` degree overflow
- Fixed SNESMOD FIFO clear
- Fixed `colorMathSetSource` CGWSEL bit 1 polarity
- Fixed write-only register reads in library code

### Examples

- 36 working examples (up from 25), 11 new:
  - **Games**: mapandobjects (object engine platformer), slopemario (slope collision)
  - **Maps**: dynamic_map (tilemap sprite engine)
  - **Graphics**: metasprite, object_size, mixed_scroll, mode1_bg3_priority, mode1_lz77, hdma_gradient
  - **Input**: mouse, superscope
  - **Basics**: collision_demo (AABB + tile collision)
- Removed variable-shift workarounds (collision_demo, background.c)
- Added GFX conversion rules to all example Makefiles (CI clean-build safe)

### Build System

- Multi-file C compilation support (`CSRC` variable)
- Automatic module dependency resolution in `common.mk`
- Bank $00 free space threshold warning in `symmap.py`

### Testing

- 57 compiler regression tests (up from 54)
- 25 unit test modules
- 36 example validations
- POSIX-compatible test scripts (macOS `grep -oE` instead of `grep -oP`)

## [0.1.0] - 2026-02-16

### Compiler (cc65816 — QBE w65816 backend)

- **-31.3% cycle count** vs PVSnesLib + 816-opt on benchmark suite
- 13 optimization phases: dead jump elimination, A-register cache, 8/16-bit mode tracking,
  leaf and non-leaf function optimization, comparison+branch fusion, dead store elimination,
  tail call optimization, and more
- Fixed unsigned integer promotion (u16 comparisons with values >= 32768)
- Fixed signed right shift (`>>` on negative values)
- Fixed function return values being clobbered by epilogue
- Fixed `__mul16` calling convention mismatch
- Added inline multiply for *3, *5, *6, *7, *9, *10
- String literals placed in ROM (saves WRAM)
- `pea.w` for constant arguments
- `.l` to `.w` address shortening for bank $00 symbols
- `stz` for zero stores to global symbols

### Library

- New modules: `mosaic`, `window`, `colormath`, `collision`, `entity`, `animation`, `sram`, `mode7`
- Modernized API with Doxygen documentation on all public headers
- `consoleInit()` sets sensible BG1 defaults (no extra setup needed for simple programs)
- Fixed CopyInitData 16-bit byte overrun (corrupted initialized static variables)
- HDMA support with direct and indirect tables

### Examples

- 25 working examples covering all subsystems:
  - **Games**: Breakout, LikeMario (platformer with scrolling)
  - **Graphics**: sprites (simple, dynamic, animated), backgrounds (Mode 1, Mode 7,
    Mode 7 perspective, continuous scroll), effects (fading, HDMA wave, gradient colors,
    transparency, window, mosaic)
  - **Audio**: SNESMOD music, sound effects
  - **Input**: two-player joypad
  - **Memory**: SRAM save/load, HiROM demo
  - **Text**: hello world, text formatting

### Build System

- Unified `make/common.mk` for all examples
- HiROM support (`USE_HIROM=1`)
- SRAM support (`USE_SRAM=1`)
- Library module selection (`LIB_MODULES=console sprite dma`)
- CI pipeline on Linux, macOS, and Windows (MSYS2)
- `make release` for SDK packaging

### Testing

- 54 compiler regression tests
- Example validation with memory overlap checking
- Multi-platform CI (Linux, macOS, Windows)

### Documentation

- Doxygen-documented public API
- Example READMEs with hardware explanations
- Progressive learning path from hello world to full games
