# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.34.0 (2026-10-06).

Sent on 2026-10-06 in `2026-10-06_to_luna_rapport-v1.33.1.md`; answered
the same day (`2026-10-06_from_luna_reponse-rapport-v1.33.1.md`) and closed by
v1.34.0, pinned that day: the bare static name (D1), the cartridge RAM of
`--power-on random` (D3); the archive sums (D2) are GitHub's `digest` field,
which our pin recipe now reads (`testing/luna.sha256`). The two below were
held back: not re-run, they go out with a reproduction ROM or not at all.

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-10-05 | No diagnostic when CFGR bit 5 (MS0, fast multiply) and CLSR bit 0 (21 MHz) are both set: « MS0 must be zero in 21MHz mode » (fullsnes `1adef8e33ff3c4e9`; ares `06c6d2324e3c6d01`: products « may sometimes be invalid » in that mode). Our launchers mask the bit, but a program writing CFGR itself runs green on luna and may multiply wrong on a console. Needed: a counter or a note in `state.gsu` (like `bus_violations`) when a MUL/FMULT executes with both bits set | v1.32.0 | a ROM writing `$A0` to `$3037` then `$01` to `$3039` and running `fmult`; `luna state … --out - \| jq .gsu` shows `cfgr: 160, clsr: true` and nothing else |
| 2026-10-05 | A DSP-1 LoROM of 2 MB (`ROM_BANKS=64`, `USE_DSP1=1`, before our build refused it) loads and runs on luna while the only 2 MB DSP-1 LoROM board (SHVC-2B3B-01) maps the DSP registers elsewhere (chips audit S5). Low priority: `make` refuses the combination since 2026-10-04 (`ROM_BANKS_MAX` 32 for DSP-1); an emulator warning on header-vs-board inconsistencies would still be a service | v1.32.0 | build with `ROM_BANKS=64 USE_DSP1=1` on a tree before `a8113dd8`, `luna state` boots it without a word |

Sent on 2026-10-08 in `2026-10-08_to_luna_rapport-v1.34.0.md` (four
requests: `assets-dump --until-frame`, the actual bytes of a failing block,
`diff --audio --align-onset`, `diff --sequence` with its prototype
`testing/frame_sequence.py`; reproduction ROMs in the exchange folder,
`2026-10-08_roms/`). Answered the same day
(`2026-10-08_from_luna_reponse-rapport-v1.34.0.md`): all four are done on
luna `develop`, to ship as v1.35.0. Our reply
(`2026-10-08_to_luna_reponse-rapport-v1.34.0_reply.md`) accepts their three
choices and lists what we replay at the pin: then `frame_sequence.py` and
its row in `luna_tooling.md` are deleted, `diff_corpus.py` gains a
`--sequence` pass, and the audio re-capture rule of `testing.md` cites
`--align-onset`. The two rows above stay held.

**2026-10-10, `diff --audio --align-onset` with more than one sound.**
`audio/echo` before and after a compiler step (ROMs in
`~/workspace/partner-reports/opensnes/2026-10-10_roms_audio-onsets/`):
`luna diff --audio --align-onset echo_before.sfc echo_after.sfc
--until-frame 300` says DIFF, max delta 21.83 % in the 1500 ms window,
onset shift +2 samples. Sample against sample the two captures are the
same signal: lag 0 from 0.6 to 1.4 s, lag 2 samples from 1.5 to 2.5 s
(sum of absolute differences over the window 87 540 unshifted, 7 542 at
lag 2), lag 0 again after 3 s. One global shift, taken from the first
sound, cannot line up a second sound that moved by a different amount,
and a 500 ms window whose edge falls on an attack turns two samples into
21 % of RMS. What we would use: the onset found per window (or per burst)
and printed, so that "same sound, each burst within N samples" reads as
MATCH with its N. Told to luna's session the same day; not blocking (we
did the lag search in twenty lines of Python for the commit message, which
is exactly the kind of script `luna_tooling.md` wants gone).
Answered within the hour: luna `develop` 260397f fits every window by
itself (`--max-shift`, default 64; the final line says "per-window shift,
max N samples"). Replayed with its release build on our two pairs: `echo`
shifts +2, 0, 0, -2, -2, 0…, max delta 0.15 %, MATCH; `speech_synth` -12
then -16 eight times, 0.77 %, MATCH. Not in a version yet (no 1.37.0
without the owner's word): the row closes at the pin that carries it,
when `testing.md` can quote the new final line.
