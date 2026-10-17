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
| 2026-10-05 → sent 2026-10-12 with `gsu_ms0.sfc`; shipped the same day on luna's `develop` `1e0df1f` as `state.gsu.ms0_at_21mhz` (2 on our ROM, 0 on the four Super FX examples; a diagnostic, not in the save states; usable under `[asserts.gsu]`). To do at the pin bump that carries it: `ms0_at_21mhz = 0` in the four Super FX manifests and `luna_runner.py --coverage` refusing a non-zero count | No diagnostic when CFGR bit 5 (MS0, fast multiply) and CLSR bit 0 (21 MHz) are both set: « MS0 must be zero in 21MHz mode » (fullsnes `1adef8e33ff3c4e9`; ares `06c6d2324e3c6d01`: products « may sometimes be invalid » in that mode). Our launchers mask the bit, but a program writing CFGR itself runs green on luna and may multiply wrong on a console. Needed: a counter or a note in `state.gsu` (like `bus_violations`) when a MUL/FMULT executes with both bits set | v1.32.0 | a ROM writing `$A0` to `$3037` then `$01` to `$3039` and running `fmult`; `luna state … --out - \| jq .gsu` shows `cfgr: 160, clsr: true` and nothing else |
| 2026-10-05 → sent 2026-10-12 with `dsp1_cube_2mb.sfc`; luna decodes the DSP by board since its `develop` `1e0df1f` (ares' choice: no save RAM `$30-$3F:8000`; RAM and ≤ 1 MB `$20-$3F:8000`; RAM and > 1 MB, SHVC-2B3B-01, `$60-$6F:0000-7FFF`; fullsnes `f8cfa059b665dc1c`). Our 2 MB ROM declares no save RAM, so it matches no board: luna loads it as a SHVC-1B0N, its DSP answers at `$30`, and a warning at load and `rom.board_note` say so — the word we asked for; it does not fail by itself. Closed | A DSP-1 LoROM of 2 MB (`ROM_BANKS=64`, `USE_DSP1=1`, before our build refused it) loads and runs on luna while the only 2 MB DSP-1 LoROM board (SHVC-2B3B-01) maps the DSP registers elsewhere (chips audit S5). Low priority: `make` refuses the combination since 2026-10-04 (`ROM_BANKS_MAX` 32 for DSP-1); an emulator warning on header-vs-board inconsistencies would still be a service | v1.32.0 | build with `ROM_BANKS=64 USE_DSP1=1` on a tree before `a8113dd8`, `luna state` boots it without a word |

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

Closed at the pin of v1.37.0 the same evening: `echo` reads "max delta
0.15 %, per-window shift, max 2 samples (searched ±64): MATCH" with the
pinned binary, and `testing.md` describes the new line. The whole suite is
green on v1.37.0 with NO reference moved — neither the cycle rows of the
library bench (`dma` 17,903, `vramc` 16,243, `vramq` 10,732, the same to
the cycle, and PVSnesLib's too, measured again), nor the NMI budget, nor
the audio hashes, nor a Super FX picture. Said to luna.

**Corrected by luna the same evening, and replayed here:** the parenthesis
we first wrote ("no burst of ours starts from a 6-cycle access…") was a
cause we had not checked, and it is false. The change exists on our ROMs:
`luna profile examples/games/tetris/tetris.sfc --until-frame 200
--frames-out f.csv`, column `dma_mclk`, frames 100 to 199 — v1.36.0: 12
bursts of 24 master cycles (288); v1.37.0: six of 40 and six of 48 (528).
luna measured the same on `superfx_game_skeleton` and `superfx_3d`. What
is true is narrower: NONE OF OUR ORACLES SEES IT (pictures, WRAM, audio
hashes, NMI budget, the library bench to the cycle). Why the bench rows
`dma` / `vramc` / `vramq` do not move is not established: either they
start their bursts from an 8-cycle access with no HDMA channel enabled, or
the profile credits the burst elsewhere than the symbols the bench sums.
If a DMA's duration ever matters to a gate of ours, the column to read is
`dma_mclk` per frame.

**Settled by luna's full report** (`2026-10-10_from_luna_duree-dma-rapport-complet.md`,
answered in `…_reply.md`): the bench rows do not move because their bursts
start from bank `$00` (an 8-cycle access) with no HDMA channel enabled,
where the old and the new formula are algebraically equal; the rows DO
contain the bursts (the first reading we had kept as possible is false).
On a SDK whose code runs in bank `$00`, v1.37.0 changes a DMA's duration
only when it is started with an HDMA channel enabled. Two things of ours
it shows: the library bench is SlowROM and its page does not say so; and four examples teach an HDMA on
channel 0, which `hdma.h` itself warns against — harmless only because
none of them transfers anything with the screen on (measured the same
evening). Both are debts, for the examples review.

**2026-10-10, night: luna v1.38.0 pinned.** `"vram:WORD"` keys in
`[checkpoint.values]` answer what we were about to ask (a block at a
checkpoint; the report was written and never sent,
`2026-10-10_to_luna_blocs-dans-un-checkpoint.md`): `backgrounds_mode4`
asserts two words of the offset row in VRAM at the 125th arrival on
WaitForVBlank, and the assertion fails on a wrong value
(`checkpoint@WaitForVBlank#125 (frame 142, line 7) values.vram:3001`).
Suite green on v1.38.0 with no reference moved. Also in it, to use next:
`luna profile --until-pc SYMBOL --hit N` (a measuring window counted in
loop turns), which is how the bench should be clocked.

**2026-10-10, observation for luna (said by message): a per-window shift
equal to the search limit.** `audio/echo` before and after the faster
sample upload: at the default `--max-shift 64` the verdict is "max delta
0.56 %, per-window shift, max 64 samples (searched ±64): MATCH". With
`--max-shift 1200` every window is at -532 or -534, max delta 0.08 %: the
sound starts ONE FRAME earlier. So a one-frame shift did not stay a DIFF
at 64 on this sound (sustained, echoing): the best fit inside the window
was good enough. What we would use: a mark on the final line when the
largest shift equals the limit ("at the search limit — widen
--max-shift"), since that is the case where the printed number is not the
real one.

Taken within the hour (luna `develop` 92c5677, for a 1.38.1 with the
doubled-arrival fix): the window prints `shift=-64 (limit)`, the final
line ends "at the search limit — widen --max-shift", the JSON gains
`shift_at_limit`; the verdict stays MATCH (a comparison of levels), which
we agreed to. To do at that pin: `testing.md` — a line carrying the mark
is not quoted in a commit before it has been run again wider — and replay
the `echo` pair.

- 2026-10-11 → **closed 2026-10-12** — the hardware multiplier had no
  latency in luna 1.38.0 (`$4216/$4217` held the full product four CPU
  cycles after the write to `$4203`; references: eight, built as it goes).
  luna's `develop` `64c771d` builds it one bit per cycle (ares `aluEdge`,
  Mesen2 agree): our probe gives `f1fec1fe01fe01fe01fe5a00` there, and a
  finer one (`pieces/2026-10-12_mulprobe_fine.*`) puts the product complete
  at 8 cycles exactly, the quotient at 16. The waits of `tcc_mul16`,
  `tcc_div16`, `mul32.asm`, `math.asm` and `fixed32.asm` were shortened
  the same day, judged by that binary. To re-run at the pin bump that
  carries it. Sent: `2026-10-11_to_luna_multiplier_latency.md`, answered by
  `2026-10-12_from_luna_multiplicateur_commite.md`, closed by
  `2026-10-12_to_luna_multiplicateur_reply.md`.

- 2026-10-11 — `luna diff --audio` on an example that plays NOISE: a codegen
  change that moves the first `KON` by 10 samples gave DIFF at 6.43 % on
  `audio/apu_switch` (per-window shift found at −96 on the noise-only
  windows, −8/−9 on the others). What settled it was two traces luna already
  has: `state --apu-log` (1,016 port writes identical in value and order
  between the two ROMs) and `state --dsp-trace` (75 DSP writes identical;
  only the SPC timestamps differ, by 313 cycles). The S-DSP has one noise
  generator, clocked by the global counter at the FLG rate (fullsnes
  `bc0eacb8a82c13c4`, anomie `65daaafcdc153ad3`), so a key-on that lands
  elsewhere in its sequence reads other values, and at slow NCK rates a
  500 ms window holds a handful of them. Asked: a `diff --dsp` (or an
  `--audio` mode) that compares the DSP write sequences of the two ROMs,
  value and order, and reports the timing shift — the right oracle for a
  noise-driven example, where a waveform RMS cannot be. Light.

## 2026-10-13 — `luna record` (luna `develop` fea4a77, not released)

Announced, nothing asked (`2026-10-13_from_luna_luna_record.md`): one run
writes a labelled video (`ffmpeg` needed; `--no-video` without it), a text
account of when the picture moves, freezes or goes dark with the pad
changes and the sound second by second, and one labelled picture a
second. Not checked on their side: PAL, overscan, the libx264 path. To do
at the pin bump that carries it: regenerate the luna reference page from
`--help`, and try it on an example's manifest script — a bug report to the
game or from it could carry the `.md` instead of a description.

2026-10-17, direct exchange, nothing asked yet. luna's debugger foundations
(MCP: `memory_kinds` / `read_memory`, `eval` in Mesen2's expression
language, `bp_add` with `condition` and `mark`, `set_break_sources`,
`step_kind`) and, on its develop 140d830, `until_if = "expr"` on an
`at_symbol` checkpoint or a poke, and `luna state --until-pc X --until-if`.
Our side: `SNES_ASSERT` (WDM $00) is our assert channel, read by
`--wdm-out` on every compare run; `set_break_sources {wdm: true}` stops at
the call site in an interactive session. Candidate, held by luna until a
prototype of ours shows the need: `[asserts.expr] never = "…"` in a
manifest, an invariant checked without code in the ROM. When we prototype
it, say which granularity it needs — per frame is nearly free, per line
about 262 evaluations a frame. To do at the pin bump that carries
140d830: try `until_if` on a manifest that counts arrivals today.
