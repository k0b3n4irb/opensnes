# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.31.0 (2026-10-03).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-10-03 | The audio oracle is a hash of the WAV (`luna run --audio-out`, `tools/luna-test/audio_regress.py`), so it flips on a shift of a few CPU cycles in the code that talks to the SPC700: three re-captures in a week for SNESMOD examples (KOF fix 2026-09-26, `snesmodProcess` and `snesmodInit` on 2026-10-03; the last one moved `snesmodInit`'s end by 3 cycles and `snesmod_music` came back to an earlier hash). Each time we proved the change benign by hand: old and new WAV, RMS per half second within 0.04 % to 1.7 %, onset within 8 samples. That comparison is computing an answer luna should give (`luna_tooling.md`), so we did it in the scratchpad and kept no script. | v1.31.0 | `luna diff --audio A.sfc B.sfc --until-frame N [--window-ms 500] [--tolerance-pct 2]` printing per-window RMS for both and MATCH / DIFF, exit 0/1 like `luna diff`; or the same on two WAV files. We would run it in the commit that re-captures an audio hash, and quote its output |
