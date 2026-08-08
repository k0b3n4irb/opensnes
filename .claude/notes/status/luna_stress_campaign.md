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

## Wave 2 — open-bus / MDR (DONE 2026-08-08)

- **luna models open-bus / MDR correctly — PASS (no finding).** A C pointer
  read always hits bank $00 (structural limit), so the MDR bank byte can only
  be exercised from asm. `stress/openbus/ob.asm` reads the $2100 mirror through
  banks via `lda.l bb:2100`; luna returns the bank byte every time
  (`3F 01 20 10`), `$00` for the control, and the real value for a readable
  register — matching Mesen2 **byte-for-byte** and the fullsnes/anomie rules.
  The earlier "all $00" was the compiler's bank-$00 `lda.l` (where $00 *is* the
  correct MDR), not a luna gap.
  → **Promoted** to a luna-only regression (`probes/open_bus.py`).
- Only divergence in the whole probe: `$213F` STAT78 — luna reports PPU2
  (5C78) version **2**, Mesen2 **3**. Chip-revision modelling choice (real
  consoles ship rev 1/2/3), not a bug → **owner question, not filed**.

## Wave 3 — luna v1.14.0 "OpenSNES DX release" adoption (2026-08-08)

luna shipped #168–#181 in v1.14.0, explicitly built for this project (native
`luna test` runner, full CLI↔MCP parity, determinism oracles, debugging API
v2, JSON `--peek`). Their release asks OpenSNES to test three things.

- **Pin bumped v1.13.0 → v1.14.0.** Validated render- and behaviour-identical:
  coverage 81 OK/0 FAIL, visual 83/83 fbhash match, probes 19/19, WRAM 81/81.
  No baseline re-capture. Committed.
- **#181 acceptance — MET.** Ported `probes/hw_math.py` to a native
  `luna test` manifest (`stress/hwmath/hwmath.toml`, `[asserts.values]` on the
  div/mul result slots incl. the ÷0 quirk + `wdm_empty`). `luna test` returns
  the same verdict: PASS on correct values, FAIL (precise expected/got) on a
  tampered value, exit contract 0/1/2 confirmed. This proves the native runner
  can subsume the Python harness → path to deleting transitory code (luna-first).
- **#175 adopted.** `probes/lib.py:peek()` now reads the structured `peeks`
  array from the `--out` JSON (`{spec, space, addr, bytes_hex}`) instead of
  regex-scraping the stderr hexdump. Behaviour-identical — all 19 probes pass
  (symbol + BANK:OFFSET + signed-word paths). The stderr dump still exists but
  the JSON is the supported channel.
- **MCP debug session — validated (3rd ask done).** `stress/mcp_probe.py`
  drives `luna mcp` over stdio (newline-delimited JSON-RPC): handshake + the
  full 94-tool catalogue, serverInfo `luna v1.14.0` (#167 handshake fix). A
  real session on `apu_switch` exercised: `resolve_symbol`, `peek_memory`
  (by symbol), `run`/`step_until_frame`, symbol-annotated `call_stack`
  (WaitForVBlank JSL frame) and `cpu_trace`, memory search sessions
  (`search_begin u8` → `refine eq/changed`, `remaining` counts narrow),
  `freeze_add` (peek returns the frozen value across a run), and the
  determinism oracles — **`frame_hash` matches the CLI `--print-fbhash`
  byte-for-byte** (`714a220e2daaa1e4`) + `wram_page_hashes`.

All three v1.14.0 release asks are now done (#181 acceptance, #175 JSON peeks,
MCP debug session).

## Wave 4 — first probe migrated to `luna test` (2026-08-08)

Started the luna-first endgame (retire the Python harness). **Migrated the
`hw_math` probe** off Python onto native `luna test` manifests:

- `stress/hwmath/hwmath.toml` + `stress/ppumul/ppumul.toml` assert the
  multiply/divide + Mode 7 result slots via `[asserts.values]` (+ `wdm_empty`).
- New `make test-manifests` target (builds the stress ROMs, runs `luna test`,
  exit 0/1/2); wired into `make tests` after the probes.
- **Deleted `probes/hw_math.py`** — the checks now live in luna's own runner.
  Probe suite 19 → 18; the migrated coverage runs via `luna test` (2 passed).
- Fixed a stale comment in `ppumul/main.c` (32767×127 = 4161409 = 0x3F7F81,
  not 0x3F7F01 — luna's result was always correct; only the comment was wrong).

Pattern proven: a fixed-value-assert probe ports 1:1 to a manifest. Next
candidates are the other pure-assert probes (controller, dma_cgram slots);
probes with Python logic (RMS, directional, JSON state) stay Python for now.

## Wave 5 — full MCP surface sweep (2026-08-08)

Answering "did you test everything v1.14.0 delivered?" — the earlier waves
covered the 3 asks + a representative slice, not the whole catalogue. So:

- **`stress/mcp_sweep.py`** calls **all 94 MCP tools** once, dependency-ordered
  (setup → action → observe → mutate → cleanup), schema-correct args:
  **94/94 OK, 0 errors, 0 not attempted.** Covers the untested surface —
  pokes (VRAM/CGRAM/OAM/ARAM/mem), all enable/take traces, breakpoints v2
  (add/list/set_enabled/remove/clear), symbols v2 (`load_symbols_str`,
  `clear_symbols`, `symbol_for_addr`, ARAM/SPC space), `wram_snapshot`,
  `loop_probe`, `render_*`, `decode_sprites`, `export_spc`, `sram_get/set`,
  `save_state`/`load_state`, `load_rom_bytes`, `set_port_device`,
  `set_cpu_register`, `run_until_pc/mem_read/mem_write/break`, etc.
- A first sweep flagged 6 "errors" — all self-inflicted: a malformed
  `load_symbols_str` text (`count:0`) **REPLACED** the symbol table and wiped
  `current_song`. Lesson recorded: `load_symbols_str` replaces, not appends.
- **#167 `search_memory` $7F fix verified**: poked `AB CD EF` into `$7F:1234`
  and searched — luna returns `0x7F1234` (correct), not the old bogus
  `$7E:1xxxx`. `poke_memory`/`peek_memory` handle bank `$7F` correctly too.

Net: the entire luna v1.14.0 delivery (94 MCP tools + the search_memory fix +
`luna test` + JSON peeks + the CLI surface exercised by probes) is validated.

## Filed / closed on luna so far
- #126 — CLOSED (verified fixed on v1.13.0).

## Candidate owner-questions (not filed)
- STAT78 PPU2 version: which 5C78 revision does luna intend to model (2 vs 3)?
- Cross-arch WRAM determinism (mapandobjects/slope_collision, root cause
  untracked): can't repro single-arch here; needs x86↔arm comparison.

## Next
- Deeper waves: mid-scanline raster/HDMA timing; DSP/audio fidelity;
  SA-1 / Super FX contention.
- Standing takeaway so far: luna v1.13.0 passed every corner tested
  (robustness, CPU math, PPU Mode 7 math, open-bus/MDR). Findings to date are
  one closed bug (#126) + hard regression coverage, not defects.
