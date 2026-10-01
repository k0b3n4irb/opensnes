# luna → OpenSNES: the four items of 2026-09-27

| | |
|---|---|
| **From** | luna (`k0b3n4irb/luna`) |
| **Re** | `2026-09-27_to_luna_reply.md` (your reply to v1.28.0, four new items) |
| **Status** | **All four done, released as `v1.29.0`.** Save-states and existing manifests are unaffected. Please also read the separate note on the SA-1 speed figure, which concerns your tutorial. |

Thank you for re-measuring R5 before quoting it — and read the SA-1 note
(`luna_note_opensnes_2026-09-27_sa1_speed.md`) before the tutorial ships:
the figure you re-measured faithfully is an upper bound, and we owe you a
corrected one.

The two withdrawn observations of the cache note are struck from ours.
Your fix to the build script (every scripted edit asserts it matched) is
the right one; we had the same lesson with a pipe that swallowed an exit
code.

---

## 2.1 `--audio-out` under `--input` — bug, fixed

Reproduced exactly: 159 936 samples without a script, 95 819 with
`--input 150:0`, 16 384 with `--input 299:0`.

**It was worse than a truncation.** The APU queue holds 16 384 samples
(~0.5 s) and refuses *new* samples once full. The input pre-roll stepped
without draining it, so the WAV kept the **first** 16 384 samples, then
jumped straight to the last checkpoint. 95 819 = 16 384 from the boot + the
~79 400 after frame 150. The WAV was not only short, it was spliced, with no
warning.

We found a second path with the same fault, which you had not hit yet: the
bridges to `--cpu-trace-from`, `--mem-trace-from`, `--dma-trace-from` and
`--superfx-trace-from` also stepped without draining.

Both now drain as they go. Checked on `snesmod_music`:

| Flags (300 frames, or `-n 3000000`) | v1.28.1 | v1.29.0 |
|---|---|---|
| none | 159 936 | 159 936, same SHA-256 as v1.28.1 |
| `--input 10:0` / `150:0` / `299:0` | 159 936 / 95 819 / 16 384 | 159 936, identical bytes |
| `--cpu-trace-from 2000000` | 27 660 | identical to no flag |
| `--dma-trace-from 2500000` | 21 392 | identical to no flag |
| `--input 150:0` + `--mem-trace-from 2800000` | 16 384 | identical to no flag |

**Your six button-driven examples can join the WAV-hash oracle.** A script
that presses buttons changes the audio, as it should; a script the ROM
never reads does not.

`luna test` was not affected: it has drained per frame since issue #211.
For API users, the pre-roll with audio is
`Emulator::run_input_script_with_audio`.

## 2.2 SA-1 BW-RAM in `srm_out` — gap, closed, and wider than SA-1

Your `libtest_sa1_sram` with `srm_out` now writes **32 768 bytes**
(`rom.sram_kb = 32`), starting `C1 D2 E3 F4`. `srm_in` works too: a `.srm`
starting `AB CD 12 34` is read back at `40:0000` before the ROM touches it
(v1.28.1 read `00`).

**The cause was general.** Only the LoROM, HiROM and S-DD1 mappers exposed
their battery RAM. The SA-1 chip and the DSP-1 wrapper returned an empty
save. That also meant the **GUI kept no `.srm`** for Super Mario RPG,
Kirby Super Star or Super Mario Kart. Both are fixed.

Two rules worth knowing:

- **An SA-1 save is the part of BW-RAM the header declares.** BW-RAM is
  never smaller than 2 KB, so a cart that declares no SRAM still has
  BW-RAM, but no save and no `.srm`. If your SA-1 fixture should persist,
  declare its size in the header, as `libtest_sa1_sram` does.
- **Super FX save RAM is still not exposed.** The header's SRAM byte
  describes battery RAM separate from the GSU work RAM, and luna does not
  model it separately yet. We will port it from a source rather than guess;
  until then a Super FX `srm_out` is 0 bytes.

## 2.3 `scheduler.last_nmi_frame` — added

The `frame_count` at which the latest NMI was delivered; `null` until one
is, after power-on, reset or a state load:

```bash
luna state --until-frame 300 --out - print_string.sfc \
  | jq '.scheduler | .frame_count - .last_nmi_frame <= 1'
# → true
```

Your negative control, `$4200 = 0` after 100 frames, is our unit test:
after `NMITIMEN.7` is cleared the field stops moving and the gap grows.
`nmi_still_alive` can go.

Two details:

- **It is stamped where the NMI is raised**, at VBlank entry, carried
  through the bus cursor. A step that spans several lines, such as a long
  DMA, still records the right frame.
- **A ROM that turns NMI off on purpose** for a while (a decompress, a
  transition) reads `false` during that window. Probe at a frame where
  your example is in its main loop.

It is not in save-states, which are unchanged; that is why a state load
resets it to `null` until the next NMI.

## 2.4 `luna test --jobs N` — added

```bash
luna test --jobs 0 tools/luna-test/manifests/
```

`0` is one per CPU; the default `1` is serial. Each manifest runs on its
own emulator. **The report is unchanged:** the same `PASS` / `FAIL` lines in
the same order, the same `--report json`, the same exit code. On your 121
manifests, on our six cores: **73 s serially, 16 s with `--jobs 0`**, and
the two stdouts are byte-identical. We checked 0, 1 and 2 as exit codes.

Only stderr (symbol-loading notes) interleaves. One difference in
failure: a malformed manifest still exits 2 with its own error, but in
parallel the others have run by the time it is reported.

---

## What changed for everyone

Nothing to migrate. Two things may look different:

- **GUI:** SA-1 and DSP-1 games now write a `.srm` beside your saves, as
  LoROM games always did.
- **WAVs:** the output of `luna state --audio-out --input …` or
  `--audio-out --*-trace-from …` is longer. It is the whole run it always
  should have been, so any baseline recorded with those flags on v1.28 was
  recorded from a spliced file and needs re-recording.

## Verified

The full local suite with `tests/roms/` populated passes (commercial
goldens and Tom Harte included); `cargo fmt` and `clippy --all-features
-D warnings` are clean, and CI is green. The save-state shape test passes:
the declared save size and `last_nmi_frame` both live outside the
serialized machine. The published binary was checked the way your
`install-luna.sh` reads it.
