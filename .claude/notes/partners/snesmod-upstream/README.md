# SNESMOD upstream (Mukunda Johnson, github.com/mukunda-/snesmod)

Not a partner in the sense of `.claude/rules/partners.md`: the author of the
SNESMOD driver and of smconv, which PVSnesLib and OpenSNES both ship. What we
find in his code goes to him as issues, one per correction, written plainly
(ASCII only, first person, no emoji). Issues only, no PR offered (owner,
2026-10-03): each issue carries what is needed to reproduce and fix. Nothing is posted without the owner
reading the final text.

Plan: `~/.claude/plans/federated-forging-flask.md` (2026-10-03).

| Draft | Subject | State |
|---|---|---|
| `2026-10-03_to_mukunda_issue-A_koff.md` | driver: ResetSound clears KOF 60 cycles after setting it, under the 64-cycle KON/KOFF poll | ready for the owner to read; not posted |
| `..._issue-B_smconv-loop-end.md` | smconv (Go): data after the loop end is kept; ping-pong unroll appended after it; resample sees the forward half only (one fix) | checked with a Go test on ramps (below); ready for the owner to read |
| `..._issue-C_surround.md` | driver: volume-column pan does not clear surround (KungFuFurby's fix of 2015, in PVSnesLib) | read in both sources, not measured, and the draft says so; ready to read |
| `..._issue-D_smconv-tuning.md` | smconv (Go): `resampleLoop` returns old/new where the C++ returned new/old | checked (pitch base -14 vs +5 on `pollen8.it` sample 17); ready to read |
| `..._issue-E_smconv-loop-start.md` | smconv (Go): `Loop = loopStart / 16 * 9` rounds down, the codec aligns the loop start up | checked with the Go test; ready to read |

Order when the owner says go: A first, then C, then B, D, E. One at a time.

## Evidence for A, re-run on 2026-10-03 (luna v1.30.4)

`examples/audio/snesmod_music/music.sfc`, stop (B, `0x8000`) or pause (X,
`0x0040`) pressed on frame F for F = 40..200, `[asserts.dsp]` V0..V7 ENVX = 0
at F+140 (stop) or F+240 (pause), `luna test --jobs`:

| Driver | stop | pause |
|---|---|---|
| ours (fixed, `8d83859f`) | 161 / 161 pass | 161 / 161 pass |
| same ROM with the 9 bytes put back (`8F 5C F2 8F 00 F3 8F 00 C1`) | 8 fail: F = 112, 122, 129, 146, 153, 162, 181, 198 | 8 fail |

Earlier counts: 8 on luna v1.24.0, 10 on v1.27.0 (commit `8d83859f`).
Mukunda's `smconv/smconv/sm_spc.bin` (commit 3e4990a): KOF = $FF at `$6D`,
the final KOF write at `$91`, one occurrence.

## What his repository has that we do not (compared 2026-10-03)

- SPC driver: nothing; PVSnesLib's copy (ours) is ahead (KungFuFurby's
  surround fix, PAUSE/RESUME).
- 65816 side: nothing.
- smconv: IT compressed samples (through `modlib`, MIT). Ported to our C
  smconv on `wip/smconv-it-compressed`.

## Evidence for B, D, E (2026-10-03, his `main` at 3e4990a, Go 1.27.1 in the scratchpad)

`2026-10-03_issue-B_loops_test.go.txt` dropped into `smconv/smconv/` as
`zz_loops_test.go`, `go test ./smconv -run TestLoopShapes -v`. Input is a
ramp (sample i = i*8); the BRR is decoded with his codec.

| Case | Output |
|---|---|
| 400 samples, forward loop 96-256 | 400 samples out, tail 256..399 kept |
| 400 samples, ping-pong 96-176 | 480 samples: 0..399 then 175..96 |
| 1878 samples, ping-pong 317-969 | tuning 0.98788 (= 652/660), 2976 samples |
| 256 samples, forward loop 100-256 | 736 samples (112 + 4 x 156), Loop field 54 (block 6) |
| 256 samples, forward loop 96-256 (control) | 256 samples, Loop 54: correct |

His C++ (`convert/source/brr.cpp`) copies only `length` samples, pads the
front to align the loop start and returns `1.0/factor`; our C smconv is a
port of it and gives, on `pollen8.it` sample 17, a 1312-sample loop where the
forward and the backward pass are both found (correlation 0.94 / 0.96 against
the source PCM, BRR decoded with the 15-bit clip of anomie's S-DSP doc).
