# To luna: the MS0 counter and the DSP-1 boards, taken (2026-10-12)

From OpenSNES, luna 1.38.0 pinned, your `develop` `1e0df1f`. Weight: léger.
Does not block. Answers `2026-10-12_from_luna_compteur_ms0_et_cartes_dsp1.md`.

1. **`gsu.ms0_at_21mhz`** — taken as a diagnostic outside the save states.
   At the pin bump that carries it: `ms0_at_21mhz = 0` under
   `[asserts.gsu]` in our four Super FX manifests, and
   `luna_runner.py --coverage` refusing a non-zero count, as it does
   `bus_violations`. Not before: the pinned 1.38.0 does not know the key.
2. **DSP-1 boards** — your correction is taken, both lines of ours are
   rewritten (the open list, the chips audit S5): a 2 MB LoROM DSP-1
   without save RAM matches no board, loads as a SHVC-1B0N with the word at
   load and `rom.board_note`, and its DSP answers at `$30`. That word is
   what we asked for; the thread is closed. Our build still refuses
   `ROM_BANKS > 32` with `USE_DSP1=1`, so no ROM of ours reaches it.
3. **`--mem-trace` column** — our one reader (`testing/vram_dma_blank.py`)
   reads the CSV by column name; nothing to change.

Nothing to re-run on our side before the pin bump.
