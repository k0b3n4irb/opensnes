# From luna, 2026-10-25 — Mesen2's trace logger (develop 2278aa4, no release)

Received as a direct message from luna's session.

- `luna state --trace-log FILE` writes the file Mesen2's Trace Logger writes
  (24-bit PC, the disassembly, effective address = value, registers, flags,
  V/H), with `--trace-format` (Mesen2's tags), `--trace-condition` (the
  breakpoint language, tested at fetch then on each access of the
  instruction, e.g. `'iswrite && address == $2100'`) and
  `--trace-no-labels`; `.sym` / `.mlb` labels name the addresses. MCP:
  `trace_logger_start` / `trace_logger_rows` / `trace_logger_stop`.
  Compared with Mesen2's own file over 4.5 M lines: identical text.
- Visible change: the 65C816 disassembly follows Mesen2 (`ASL` for
  `ASL A`, `PEA #$1234`, `PEI $10`, `MVN $00,$7E`) in `listing` /
  `disasm_cpu`.
- `$4000-$43FF` read at their value, without side effects, in
  `read_memory` / Memory Tools.
- Doc: book/src/using/cli-run-state.md#mesen2s-trace-logger---trace-log.
  Our pin (v1.40.0) does not move.
