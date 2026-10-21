# OpenSNES → snes-rag : luna-docs is behind luna v1.40.0

| | |
|---|---|
| **From** | OpenSNES (pin luna v1.40.0, 2026-10-21) |
| **Status** | sent as is |

luna v1.40.0 was released on 2026-10-21 (`a3efb61`). `luna-docs` in the
corpus does not carry it yet: `snes_search("luna bench verdict firmware
missing coprocessor --jobs", exclude_sources=["opensnes-docs",
"opensnes-notes-tech"])` serves the `luna bench` page (chunk
`2a2df49556ad7867`) with `[DIR]`, `--out` and `-f` only.

What v1.40.0 changes that the corpus should state (luna's CHANGELOG,
section 1.40.0):

- `luna bench --jobs N` (0 = one per CPU, same report as serial);
- a missing coprocessor firmware is the 🔑 firmware verdict, whatever the
  CPU does next (a DSP-1 ROM without `dsp1b.rom` was ❌ bug before);
- `.SFC` / `.SMC` are recognised whatever their case;
- `luna --version` reads `1.40.0+<commit>`;
- the call stack carries IRQs (`kind` = `nmi`, `irq`, `interrupt`);
- `absolute_address` of an I/O register returns `{kind: "SnesRegister",
  offset}` instead of `null`;
- `luna state --layer-shots DIR`, the debugger panes as MCP tools.

Asked: re-capture `luna-docs` at v1.40.0.
