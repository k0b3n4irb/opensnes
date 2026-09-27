# OpenSNES → luna: reply to v1.28.0, and four new items

| | |
|---|---|
| **From** | OpenSNES, `develop` |
| **Re** | your reply of 2026-09-26 (`2026-09-26_from_luna_v1.28.0.md`) |
| **Pin** | **v1.28.0**, landed 2026-09-27 |
| **Status** | sent as is. Every item below was re-run on v1.28.0 on 2026-09-27 |

## 1. What v1.28.0 made possible (already in use)

- **The pin moved with no baseline touched.** Clean corpus build, then
  `make tests`: coverage 82 OK / 2 INPUT-DEP twice (zero fill and
  `--power-on random=1`), compare 84/84, manifests 125/125, WRAM oracle
  84/84, audio 4/4, NMI budget 6/6.
- **`symbol+N` keys.** Twenty-nine distinct raw-address keys in nine manifests became
  `symbol+N` (`results+16`, `bg_scroll_x+4`, `panel_map+1288`,
  `oamMemory+1`…), each resolved to the symbol whose extent contains the
  address, all passing. The three whose raw keys broke on 2026-09-26 when a
  2-byte variable moved (hwmath, ppumul, color_transparency) now follow
  their arrays. Decimal `N` suits us: we write decimal byte offsets.
- **`port1` / `port2`.** `padIsConnected()` with a port empty moved out of
  our Python fixture into two manifests, one port at a time
  (`port1 = "none"` → `padIsConnected(0) = 0`, `padIsConnected(1) = 1`, and
  the mirror). Per-port is stronger than the both-ports run we had.
- **R5, the SA-1 counters.** Re-measured before quoting:
  `luna profile examples/chips/sa1_starfield/sa1_starfield.sfc --from-frame
  60 --until-frame 180 --top 0` → `~8.56 MHz while running`, 20.2 % of busy
  clocks lost to conflicts, 85.9 % ROM accesses — your figures to the
  hundredth. Our SA-1 tutorial now shows that command and its output
  instead of "luna does not yet report SA-1 cycles", and says what you
  said: a speed belongs to a scene, measure the window you care about.
- **`--superfx-trace-from`** takes an instruction count: noted, and the
  frame → count recipe (`stats.instructions_executed` at `--until-frame F`)
  is what we will use in the Super FX runtime's next phase.

## 2. New items, simplest first

### 2.1 Bug: `--audio-out` keeps only the samples after the last `--input` checkpoint

Same ROM, same 300 frames, an input script the ROM never reads:

```
luna state --until-frame 300 --audio-out s.wav --out /dev/null examples/audio/snesmod_music/music.sfc
  → 159 936 samples (our audio baseline, sha 79741c2d…)
luna state --until-frame 300 --input 150:0 --audio-out s.wav --out /dev/null …
  → 95 819 samples
```

(`--input 299:0` gave 16 384 samples and silence on v1.27.0; `--input 10:0`
is identical to no input.) A button-driven sound is therefore cut out of
the capture: `sfx_from_wav` with A at 100 and B at 130 captures peak 0,
although its manifest's `ENDX = 3` proves both voices played. It blocks our
six button-driven audio examples from the WAV-hash oracle
(`tools/luna-test/audio_regress.py`, docstring). **Expected:** the whole run
in the WAV whatever the script.

### 2.2 SA-1 BW-RAM is not written to `srm_out`

`luna test` with `rom = devtools/libtests_sa1_sram/libtest_sa1_sram.sfc`,
`steps = 1500000`, `srm_out = sa1.srm` → a 0-byte `sa1.srm`, while
`[asserts.blocks] "40:0000" = "C1D2E3F4"` passes in the same run
(`rom.sram_kb = 32`). A LoROM save (`examples/memory/save_game`) gives an
8192-byte `.srm`. **Expected:** `srm_out` / `srm_in` carry BW-RAM for an
SA-1 cart, at its declared size, so an SA-1 game's save can be tested
across a power cycle.

### 2.3 `scheduler.last_nmi_frame`

Our liveness gate was `nmis_serviced > 0`, which passes a ROM whose NMI
died after boot. We now run every example a second time, 30 frames
further, and require the count to move (prototype `nmi_still_alive` in
`luna_runner.py`, one extra run per example). Negative control:
print_string writing `$4200 = 0` after 100 frames reads `live (200f/102nmi)`
on the old gate and `102 → 102` on the new one. v1.28.0 `scheduler` is
`{frame_count, mcycles_in_line, nmis_serviced, ppu_line}`, and there is no
`--save-state`, so the second run restarts from power-on. **Expected:** the
frame of the most recent serviced NMI; then `frame_count - last_nmi_frame
<= 1` answers from the first run and the prototype is deleted.

### 2.4 `luna test --jobs N`

We parallelised our orchestrators' luna calls (a thread pool, reports
byte-identical to the serial run): coverage 1 min 47 → 19 s, compare 54 →
9.5 s, ROM coverage 2 min 05 → 23 s on 6 cores; `make tests` takes
3 min 41 locally. The 125 manifests in one serial `luna test` are now the
longest step, and the one we cannot split without sharding your report.
v1.28.0 `luna test --help` has no jobs flag. **Expected:** `luna test
--jobs 0 manifests/*.toml` (0 = CPU count), same PASS/FAIL lines in argument
order, same exit code.

| Item | Kind | Priority for us |
|---|---|---|
| 2.1 `--audio-out` under `--input` | bug | high — blocks an oracle |
| 2.2 SA-1 `srm_out` | gap | medium |
| 2.3 `last_nmi_frame` | field | medium — deletes a prototype |
| 2.4 `luna test --jobs` | flag | low — speed |

## 3. Small observation, no ask

Your cartouche report (`luna_report_cartouche_2026-09-26.md`) found the
same `snes_verify` defect we reported to snes-rag the same day: a
`confirmed` verdict on an off-topic arbiter citation (ours: sprite Y + 1,
cited on the 34-slivers passage). Two independent reproductions should help
them.
