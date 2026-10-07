# Silent defects log — freeze criterion 7

**What this is.** Freeze criterion 7 (owner decision 2026-10-03, plan
approved the same day): *two weeks without discovering a silent defect in
the library, the runtime or the compiler, fixed or not.* A silent defect is
a wrong behaviour that no build step and no test reported — the red and
orange classes of `KNOWN_LIMITATIONS.md`. The criterion as first written
("no new orange entry in `KNOWN_LIMITATIONS.md`") counted only the defects
left open; every defect of the table below was fixed the day it was found,
so it would have been met while we were still finding one every other day.

**How to read it.** One line per defect, by date of discovery. The
criterion is met fourteen days after the last line, **counted from the day
the hunting campaign ends** (below): a quiet fortnight proves nothing if
nobody was looking.

**Effort during the window (2026-10-04, governance audit rec 5).** The
fortnight counts only if someone keeps looking. The minimum, named here so
it can be checked: every week of the window, (a) `make tests` on a clean
tree plus `luna_runner.py --coverage --power-on random=N` with a new seed,
`make test-pal` and `make luna-bench`; (b) one public header read against
its code, chosen by the oldest "last read" date in the table at the end of
this file; (c) the partner reports of the week answered. Each week's effort
is logged below the table with its date; a week without a line restarts the
count.

**What goes in.** Wrong output, wrong state or lost data in `lib/`,
`templates/`, `compiler/` or `make/`, found by any means. **What does not:**
a wrong sentence in a doc with correct code, a missing feature, a tool
crash that stops the build (loud, not silent), a defect of a partner.

## The log

| Found | Defect | How it was found | Fix |
|---|---|---|---|
| 2026-09-26 | SNESMOD `ResetSound` wrote KOF=$FF then KOF=$00 60 SPC cycles apart, under the S-DSP's 64-cycle poll: stop and pause could leave a voice sounding | reading the driver against anomie-sdsp | `8d83859f` |
| 2026-09-29 | `gsuDmaFullFrame()` wrote a third of the framebuffer on visible lines | `vram_dma_blank.py` (luna `--dma-trace`) | `7a80ac1f` |
| 2026-10-02 | Offset-per-tile off since 2026-09-12: the NMI wrote BG3's VOFS as `y - 1`, but in Modes 2/4/6 BG3 is the offset table | luna's report on `backgrounds/mode6` | `9c34b8cd` |
| 2026-10-03 | `snesmodProcess` latched the H/V counters and read OPVCT once per turn; its wait was not five lines; a full queue was overwritten | CPU trace on luna while reading the 65816 side | `0ff69aad` |
| 2026-10-03 | `snesmodInit` ended on `lda #$81 / sta $4200`: an armed H/V timer IRQ was dropped | same reading | `74b9aeab` |
| 2026-10-03 | `oamDrawMetaFlip` assumed 16-pixel pieces when large, 8 when small, whatever the OBJSEL mode: a flipped metasprite of 32-pixel pieces was drawn 16 pixels off | writing its replacement | `79c5ec73` (the new `oamDrawMetasprite` takes the piece size; the old function ships unchanged until 1.0) |
| 2026-09-26 | the dynamic sprite engine drew its sprites one line lower than `oamSet` (no `y - 1`) | reading after the VOFS change | `2981fe0b` (added 2026-10-03 from the testing audit) |
| 2026-09-27 | a Super FX job started with its IRQ on STOP unmasked and the I flag clear locked the CPU in its IRQ entry | the GSU fixture | `20638bc3` (added 2026-10-03) |
| 2026-09-27 | `gsuSetupHdmaBlanking` wrote HDMAEN bare: every other channel off, and the next `hdmaEnable` switched it off | reading | `73a5644e` (added 2026-10-03) |
| 2026-10-02 | `mode7SetScale(0x0100)` magnified twice: the helpers divided the scale by two | reading the header against the code | `93ff5e2d` (added 2026-10-03) |
| 2026-10-03 | cproc: `++` / `--` on a `FAR` object read bank $7E and wrote bank $00 (the store took the expression's empty qualifier); `reg++` on a volatile stored without `volat` | compiler audit of the campaign, reproduced on luna | `b2967ad2` (fixture `d_quals`) |
| 2026-10-03 | cproc: a whole-struct copy with a 4-aligned member copied two bytes of every four (upstream chunk table: `w` = 4 bytes) | same | `b2967ad2` |
| 2026-10-03 | cproc: `= {0}` on a `u32` array or a struct with an `s32` left the upper halves unwritten | same | `b2967ad2` |
| 2026-10-03 | cproc: a bit-field of a `FAR` object, or of const data read through a pointer, was read in bank $00 | same | `b2967ad2` |
| 2026-10-03 | map module: `mapVblank` wrote `y - 2` to BG1/BG2 VOFS (PVSnesLib's `dispyofs` is already `y - 1`, and 2026-09-12 added a second `dec a`): the map one line too low against the sprites since then | library audit; luna `state`: BG1 `v_scroll` 1022 where BG2-4 read 1023 | `a3190b13` |
| 2026-10-03 | `oamHide` / `oamClear` parked sprites at X = 256, which the PPU counts as X = 0 for its range and time tests (anomie-regs `2304edd2bf6755b9`): a hidden 32- or 64-pixel sprite still spent a slot and tiles on lines 0-47 | library audit + arbiter | `a73e0dbd` |
| 2026-10-03 | `gsuLaunch` / `gsuStartCached` wrote CFGR with MS0 (fast multiply) set while selecting 21 MHz; fullsnes `1adef8e33ff3c4e9`: "MS0 must be zero in 21MHz mode" (`superfx_hello` passes `$A0`) | chips audit + arbiter | `85483738` |
| 2026-10-03 | `consoleInit` read OPHCT/OPVCT without latching them: the H/V part of the RNG seed was 0, the same sequence every boot | library audit; luna: `rand_seed` identical under three power-on states | `81ce2912` |
| 2026-10-03 | `fixLerp` computed `b - a` on 16 bits and took bit 15 as the sign: two values 128.0 or more apart interpolated the wrong way | library audit, arithmetic | `b44b6b45` |
| 2026-10-03 | `gsuSetupHdmaBlanking(0, n)`: a top band of 0 wrote a count of 0 as the table's first entry, which ends the table; the whole frame was then DMAed on visible lines | chips audit, luna `--dma-trace`: 1 844 726 of 2 326 528 VRAM bytes outside blank | `b855ab08` |
| 2026-10-03 | build: `ROM_BANKS` had no upper bound — LoROM 127 put a string literal in WRAM (`$7E:8000`), SA-1 65 put `.rodata` in BW-RAM, the build and `check_bank_reads` stayed green and the text vanished; `GSU_RAM_KB=48` declared 32 KB | build audit, reproduced (luna screenshots) | `c06f4579` |
| 2026-10-03 | build: a changed `GSU_BANK` kept the old GSU program (`.sfx.bin` did not depend on the configuration stamp); `make USE_HIROM=1` after a LoROM build gave a LoROM ROM | build audit, reproduced (`cmp -l`) | `c06f4579` |
| 2026-10-03 | cc65816: the host preprocessor's macros and `<stdint.h>` — `int32_t` 2 bytes, `int64_t` 4, and a ROM that depends on the build machine | build audit, reproduced (`.dw` of `sizeof`) | `70943ae3` |
| 2026-10-03 | `sa1_patch` left every SA-1 ROM's header checksum off by +3 | build audit, reproduced (checker) | `f02958f1` |
| 2026-10-03 | `smconv` wrote a truncated soundbank with exit code 0 on a module with more than 8 channels or too big for SPC RAM | build audit, reproduced (12-channel IT) | `f02958f1` |
| 2026-10-03 | dynamic sprite engine: the VRAM upload queue (128 entries) had no bound; the 129th pending refresh overwrote `.dynamic_sprite_state` | library audit, code | `ee50fbc8` |
| 2026-10-03 | `hdmaIrisWipe` / `hdmaBrightnessGradient` / `hdmaColorGradient` re-called while running restarted the table at the next HBlank: one frame with the top of the table on the bottom of the screen | library audit, luna (`hdma_helpers` f150) | `e92a46c8` |
| 2026-10-03 | `audioSetVoiceVolume` passed 128-255 through to a signed DSP volume: inverted phase | library audit, code | `d853d63f` |
| 2026-10-03 | `apuWaitBoot` waited for ever when the IPL was not running; `audioInit` promised `AUDIO_ERR_TIMEOUT` | library audit, code; libtest `r_apu_boot_again` | `d853d63f` |
| 2026-10-03 | `setMode(mode | flags, 0)` dropped the flag bits of `mode` (BG3 priority) | library audit, code | `65368590` |
| 2026-10-03 | `colorMathInit` wrote COLDATA = 0, which selects no plane: the fixed colour stayed | library audit, code | `65368590` |
| 2026-10-03 | `windowCentered(w, 1)` gave an empty window, odd widths lost a pixel; `windowSplit(0)` left a one-pixel window | library audit, code | `65368590` |
| 2026-10-03 | `apuUpload(…, 0)` uploaded 65 536 bytes; a 17th `snesmodLoadEffect` returned 16, played as effect 0 | library audit, code | `d853d63f`, `b101ead2` |
| 2026-10-03 | SA-1: `.sa1_boot` was `SUPERFREE`; a big program landed in bank 1 and the 16-bit reset vector could not reach it — the SA-1 never booted, no error | chips audit, reproduced (variant of `sa1_hello`) | `208694a7` |
| 2026-10-03 | `audioLoadSample` with size ≡ 1 (mod 256): the end-of-stream handshake raced on an index echo of 0 — timeout and a hung driver | library audit, code; libtest `r_audio_load513` | `8eeda9ce` |
| 2026-10-03 | `hdmaWaveStop` wrote HOFS = 0 over the layer's real scroll | library audit, code | `752742bf` |
| 2026-10-03 | `UNFIX_ROUND(x)` overflowed its 16-bit sum from 127.5 | library audit, arithmetic | `1b4ede5a` |
| 2026-10-03 | `gsuPresentInit` accepted buffers past the first 64 KB of a 128 KB board, which its 16-bit DMA source cannot reach | chips audit, code | `1b4ede5a` |
| 2026-10-04 | `AUDIO_PAN_CENTER` (8) gave L = 7/15, R = 8/15 of the volume: the centre was not centred | library audit row 20, arithmetic | `a93a9301` |
| 2026-10-04 | `sramSave` with a source in ROM bank $00 at or above `$2000` copied WRAM `$7E:xxxx` instead (fast path took every bank-0 pointer for the mirror) | library audit row 22, code; libtest `r_sram_rom0` (negative control: 0x00 before) | `fe43b4f7` |
| 2026-10-04 | `objNew(type >= 64)` indexed past the type tables and returned a live handle | library audit row 24, code; libtest `r_obj_type64` (negative control: handle 0x0102 before) | `9a36bf8c` |
| 2026-10-04 | gfx4snes: more than 1024 BG tiles carried into the palette bits; more than 256 Mode 7 tiles truncated to a byte; a sprite/font tile (no map) mixing two palette banks drew its odd pixels with the wrong colour — all converted without a word | build-tools audit S7/S8/S12, code; three refused fixtures (exit 0 before) | `a0d26613` |
| 2026-10-04 | `mode7_flying` (379 distinct tiles, 123 entries wrapped) and `mode7_racing` (406, 150) for 256-tile Mode 7 maps: wrong tiles on screen since the examples' creation | found by the gfx4snes refusal at the corpus rebuild | `edd54f0f` |
| 2026-10-05 | Object engine: a slot index of 80 or more (`OB_MAX`) given to `objCollidObj`, `objCollidMap`, `objCollidMap1D`, `objCollidMapWithSlopes` or `objUpdateXY` was scaled by 64 and addressed the engine's own state past the pool — slot 106's `xvel` is `objunused`, the free-list head, so `objCollidMap1D(106)` under friction zeroed it and the next `objNew` handed out slot 0 again | reading `object.asm` against the library audit (B l.24); pinned by the libtest vector `r_obj_oob_idx`, red on the previous `object.asm` | `47f40f7f` |
| 2026-10-05 | `objCollidObj`, `mapGetMetaTile`, `mapGetMetaTilesProp`, `profileColorStart`, `dsp1SetCamera`, `dsp1Raster` wrote `tcc__r0` / `tcc__r9` with absolute or long addressing: called from an `nmiSet()` callback (whose direct page is the NMI's own register copy) they clobbered the interrupted main thread's scratch through the `$7E` mirror — a wrong intermediate in whatever C expression the NMI cut | reading the ASM against the library audit's l.21 (which named `profileColorStart` and `dsp1`); addressing-mode scan of every `tcc__r` write in `lib/` | `36b5c03f` |
| 2026-10-05 | gfx4snes `-m`: the map entry's palette bank came from the tile's first pixel, and index 0 is transparent in every bank — a bank-2 tile starting transparent was drawn in bank 0's colours (63 of 64 pixels) | the build-tools audit's open question, measured with a pixel oracle on a four-tile image (`banks.png`); fixture `ROUNDTRIP` in the golden suite | `7a282da7` |
| 2026-10-05 | gfx4snes `-a`: the palette rearrangement ran on the row-major image while the tiles had been converted before — `.pal` reordered, `.pic` on the old indices; `color/transparency`, the one user, decoded 51 056 of 52 509 opaque pixels to the wrong colour since the port (PVSnesLib's tool has the same order) | same oracle, 191 of 256 pixels on the fixture; the example decoded against its own `.bmp` | `7a282da7` |
| 2026-10-07 | `DECLARE_ANIM_CLIP` with 256 frames or more: the count was cast to the `u8` `len` — 256 gave 0 and `animPlay()` stopped the player, 300 gave a 44-frame clip; no error | weekly header read (`anim.h`); refusal fixture `negative/anim_clip_256` (compiled with exit 0 before) | this commit (`_Static_assert` in the macro) |

## Campaign closed — 2026-10-05

The hunting campaign the plan of 2026-10-03 required before the window
(the eight audit reports of 2026-10-03, the header-by-header reading, the
tools pushed further) ends today: every owner-independent finding of
`.claude/notes/reviews/2026-10-03_audit/2026-10-04_reste_a_traiter.md` is
closed or handed to its owner (bus factor, assets, the five-argument
functions, the console session). The table above holds the defects it
found; the last four are dated today. **The fourteen-day window of
criterion 7 opens today and closes no earlier than 2026-10-19**, and it
restarts at every new row. The weekly effort it requires is the paragraph
at the top of this file; the first weekly line is due by 2026-10-12.

**Restarted 2026-10-07** by the `DECLARE_ANIM_CLIP` row: the window now
closes no earlier than **2026-10-21**.

### Weekly effort

| Week | Date | (a) suites | (b) header read | (c) partners |
|---|---|---|---|---|
| 1 | 2026-10-07 | `make clean && make`, `make tests` green; `--coverage --power-on random=2026`: 84 OK / 2 INPUT-DEP / 0 dead of 86; `make luna-bench`: 0 bug; `make hardware-preflight`: 26 of 26 rows; `make test-pal` **failed** at its last step (the PAL-header manifest path still assumed the harness three levels deep, wrong since the move of 2026-10-05, `75d0d942`) — a loud recipe fault, fixed, then green (84 OK / 2 INPUT-DEP, 6 + 1 manifests) | `anim.h`: one defect (row above), one cost figure five to seven times too low, re-measured | luna's reply to the v1.33.1 report answered 2026-10-06, pin v1.34.0; nothing new from snes-rag |


## The hunting campaign

Opened 2026-10-03. The fortnight starts the day it closes.

| Step | State |
|---|---|
| The eight audit agents, reports in `reviews/2026-10-03_audit/` | done 2026-10-03 (examples report pending at the time of writing) |
| Header-by-header reading, promise of the header against the body of the function | to do |
| Existing tools pushed further | done 2026-10-03: seeds 7, 42, 1337 (and the testing auditor's 2, 42, 31337, `ones`): 89/89 alive, images 89/89; `make test-pal` 89/89 + 241 vectors; `make luna-bench` 34 ok, 0 bug, 55 suspect (static screens). Nothing found by them; `make test-sanitizers` and `make fuzz` not rerun (CI runs them) |

**Campaign closed:** not yet. **Fortnight ends:** not started.

## Headers read against their code

The "doc against code" read of criterion 7's window (one header a week,
oldest first). The library audit of 2026-10-03 read them all; later reads
replace the date.

| header | last read |
|---|---|
| `anim.h` | 2026-10-07 (weekly read: one defect, one wrong cost figure) |
| `apu.h` | 2026-10-03 (library audit) |
| `asset.h` | 2026-10-03 (library audit) |
| `audio.h` | 2026-10-03 (library audit) |
| `background.h` | 2026-10-03 (library audit) |
| `collision.h` | 2026-10-03 (library audit) |
| `colormath.h` | 2026-10-03 (library audit) |
| `console.h` | 2026-10-03 (library audit) |
| `debug.h` | 2026-10-03 (library audit) |
| `dma.h` | 2026-10-03 (library audit) |
| `dsp1.h` | 2026-10-03 (library audit) |
| `fixed32.h` | 2026-10-03 (library audit) |
| `gameloop.h` | 2026-10-03 (library audit) |
| `hdma.h` | 2026-10-03 (library audit) |
| `input.h` | 2026-10-03 (library audit) |
| `interrupt.h` | 2026-10-03 (library audit) |
| `lzss.h` | 2026-10-03 (library audit) |
| `map.h` | 2026-10-03 (library audit) |
| `math.h` | 2026-10-03 (library audit) |
| `mode7.h` | 2026-10-03 (library audit) |
| `mosaic.h` | 2026-10-03 (library audit) |
| `object.h` | 2026-10-03 (library audit) |
| `panel.h` | 2026-10-03 (library audit) |
| `profile.h` | 2026-10-03 (library audit) |
| `registers.h` | 2026-10-03 (library audit) |
| `sa1.h` | 2026-10-03 (library audit) |
| `scene.h` | 2026-10-03 (library audit) |
| `snesmod.h` | 2026-10-03 (library audit) |
| `sprite.h` | 2026-10-03 (library audit) |
| `sram.h` | 2026-10-03 (library audit) |
| `superfx.h` | 2026-10-03 (library audit) |
| `system.h` | 2026-10-03 (library audit) |
| `text.h` | 2026-10-03 (library audit) |
| `tile.h` | 2026-10-03 (library audit) |
| `types.h` | 2026-10-03 (library audit) |
| `video.h` | 2026-10-03 (library audit) |
| `window.h` | 2026-10-03 (library audit) |

