# luna stress-test campaign — running log

Goal (owner challenge, 2026-08-08): éprouver luna à son maximum — surface
weaknesses/gaps so luna becomes the project's ultimate, most reliable tool.
Each *validated* finding → a detailed issue on `k0b3n4irb/luna` (owner-validate
first, per `.claude/rules/luna_tooling.md`). Plan:
`~/.claude/plans/jolly-wiggling-pascal.md`.

Method per finding: repro (SDK micro-ROM) → observe via luna → oracle
(differential vs **Mesen2** MCP + fullsnes/anomie reference) → classify →
dedup vs open/closed luna issues → owner-validate → file. Pinned luna: **v1.13.0**.

## Wave 1 (2026-08-08)

- **Robustness gauntlet — PASS (no finding).** Empty / 1-byte / truncated /
  garbage (1K–64K) / text-as-ROM / missing file / forced garbage execution
  (LoROM/HiROM/SuperFX) / extreme `-n`. Zero panics, zero hangs, precise error
  messages, clean exit codes. luna is solid here.
- **luna#126 (input replay) — FIXED on v1.13.0 → CLOSED.** Filed on v1.9.0;
  re-tested both symptoms (boot-latch + periodic re-fire) on `apu_switch` with
  frame-exact repro. Both gone; checkpoint applied at the scheduled frame and
  held once. Posted verification repro + closed the issue.
  → *Downstream repo TODO*: re-enable the edge-count-sensitive asserts the
  harness disabled as #126 workarounds (`probes/apu_switch.py` cello→drums
  direction, `probes/soundboard.py` release-checkpoint window).
- **CPU multiply/divide accuracy — bit-exact (no finding).** `stress/hwmath`;
  incl. ÷0 quirk. luna == Mesen2 == reference, byte-for-byte.
- **PPU Mode 7 signed multiply — bit-exact (no finding).** `stress/ppumul`;
  signed 16×8 → signed 24-bit. luna == Mesen2 == reference.
  → **Promoted** hwmath + ppumul to a permanent luna-only regression probe
  (`probes/hw_math.py`, `luna --assert` on the verified blocks).

Key realisation: several harness-documented "gaps" were already shipped
(state `--input`+`--print-fbhash` #85, `--native-res` #115, `--force-mapper`
#95, `bench` freeze/dead-APU detector) or already fixed (#126, #107, #109).
The harness docs trail the pin (README says v1.1.0). luna is more mature than
its internal reputation.

## Wave 2 — open-bus / MDR (in progress, 2026-08-08)

- `stress/openbus`: luna matches Mesen2 on all open-bus reads except `$213F`
  STAT78 — luna reports PPU2 (5C78) version **2**, Mesen2 reports **3**. This
  is a chip-revision modelling choice (real consoles ship rev 1/2/3), not a
  clear bug → **owner question, not a filed defect**.
- Write-only/unmapped reads return `$00` in both emus, which is *correct* for
  the `lda.l` long addressing the C compiler emits (open bus = bank byte `$00`).
- **Follow-up**: a definitive MDR test needs **absolute-addressing asm** so the
  open-bus value is the operand high byte (non-zero). Not yet written.
  `openbus/` stays a transitory prototype (not promoted).

## Filed / closed on luna so far
- #126 — CLOSED (verified fixed on v1.13.0).

## Candidate owner-questions (not filed)
- STAT78 PPU2 version: which 5C78 revision does luna intend to model (2 vs 3)?
- Cross-arch WRAM determinism (mapandobjects/slope_collision, root cause
  untracked): can't repro single-arch here; needs x86↔arm comparison.

## Next
- Finish the open-bus asm test (controlled non-zero MDR).
- Deeper waves: mid-scanline raster/HDMA timing; DSP/audio fidelity;
  SA-1 / Super FX contention.
