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
- **`openbus/`** — open-bus / MDR reads (`ob.asm`, since a C read can't leave
  bank $00). Reads the $2100 mirror through banks via `lda.l bb:2100`; luna
  returns the bank byte (the MDR) every time, matching Mesen2 + fullsnes.
  Locked by `../probes/open_bus.py`.

## Prototypes (transitory)

- **`mcp_probe.py`** — a minimal MCP-over-stdio client that drives `luna mcp`
  (newline-delimited JSON-RPC): handshake, `tools/list`, and `tools/call`.
  Used to validate luna's MCP debug surface end-to-end (call stack, traces,
  search sessions, freezes, determinism oracles). Reference client + smoke
  test; not a standing regression.

## Notes

- The one open observation not locked: `$213F` STAT78 PPU2 (5C78) version —
  luna reports 2, Mesen2 reports 3. A chip-revision modelling choice (real
  consoles ship rev 1/2/3), so it is an owner question, not asserted here.

Build artifacts (`*.sfc`, `*.sym`, `*.o`, `build/`) are gitignored; only the
`main.c` + `Makefile` sources are tracked. The probe rebuilds ROMs on demand.
