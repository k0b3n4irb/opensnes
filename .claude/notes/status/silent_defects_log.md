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

## The hunting campaign

Opened 2026-10-03. The fortnight starts the day it closes.

| Step | State |
|---|---|
| The eight audit agents, reports in `reviews/2026-10-03_audit/` | done 2026-10-03 (examples report pending at the time of writing) |
| Header-by-header reading, promise of the header against the body of the function | to do |
| Existing tools pushed further | done 2026-10-03: seeds 7, 42, 1337 (and the testing auditor's 2, 42, 31337, `ones`): 89/89 alive, images 89/89; `make test-pal` 89/89 + 241 vectors; `make luna-bench` 34 ok, 0 bug, 55 suspect (static screens). Nothing found by them; `make test-sanitizers` and `make fuzz` not rerun (CI runs them) |

**Campaign closed:** not yet. **Fortnight ends:** not started.
