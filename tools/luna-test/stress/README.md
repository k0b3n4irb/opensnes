# luna stress-test scaffold

Purpose-built micro-ROMs that push luna into hard hardware corners, to
find weaknesses and gaps and make luna the project's most reliable tool.
See `.claude/notes/status/luna_stress_campaign.md` for the running log and
`.claude/rules/luna_tooling.md` for the prototype lifecycle.

Each probe writes its results into a WRAM array so the outcome can be read
byte-for-byte by BOTH luna (`state --peek`/`--assert`) and, as a one-time
validation oracle, Mesen2 (MCP `mem_read`) — a differential that turns
"is luna wrong?" into "luna=X, Mesen2=Y, reference=Z" (the project's
anti-bogus-issue safeguard).

## Promoted (permanent luna-only regression fixtures)

Verified bit-exact vs Mesen2 + fullsnes/anomie, now locked by
`../probes/hw_math.py` (`luna --assert` on the known-correct blocks):

- **`hwmath/`** — CPU multiply/divide ($4202/$4203→$4216, $4204-6→$4214/$4216),
  including divide-by-zero (quotient=$FFFF, remainder=dividend).
- **`ppumul/`** — PPU Mode 7 signed multiply ($211B M7A × $211C M7B →
  signed 24-bit at $2134/$2135/$2136).

## Transitory prototypes (investigation ongoing)

- **`openbus/`** — open-bus / MDR reads. luna matches Mesen2 on every read
  except `$213F` STAT78 (PPU2 5C78 version: luna=2, Mesen2=3 — a chip-revision
  modelling choice, not a clear bug). The write-only/unmapped reads return the
  `lda.l` bank byte `$00`, which is correct for long addressing — a definitive
  MDR test needs **absolute-addressing in asm** (operand high byte → non-zero
  MDR). Follow-up pending; not promoted (would lock possibly-wrong behaviour).

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `build/`) are gitignored; only the
`main.c` + `Makefile` sources are tracked. The probe rebuilds ROMs on demand.
