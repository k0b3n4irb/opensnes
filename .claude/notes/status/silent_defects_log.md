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

## The hunting campaign

Opened 2026-10-03. The fortnight starts the day it closes.

| Step | State |
|---|---|
| The eight audit agents, report in `reviews/` compared with the 2026-09-26 one | to do |
| Header-by-header reading, promise of the header against the body of the function | to do |
| Existing tools pushed further: several `--power-on random` seeds, `make test-pal`, `make luna-bench`, `make test-sanitizers`, `make fuzz` | to do |

**Campaign closed:** not yet. **Fortnight ends:** not started.
