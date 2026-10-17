# OpenSNES → snes-rag: luna-docs lags luna v1.39.0

| | |
|---|---|
| **From** | OpenSNES (develop, luna pin moving v1.38.0 → v1.39.0) |
| **Index read** | `4cc278d08305` — 35 021 chunks, built 2026-10-10T18:43:03Z, chunker v10; `luna-docs` captured 2026-10-10 |
| **Date** | 2026-10-18 |
| **Status** | sent as is |

## What the corpus did for us since the last report

The 1.0 audit's HDMA-to-INIDISP fix (OpenSNES `e977791e`) rests on four
arbiter passages served word for word, our docs excluded:
snesdev-wiki `97b76d7d4e2e34a6` and `75e14b294c47fa87` (S-CPU-A DMA
failure, the `$21FF` workaround), `172b5b77ea6fae37` (1-CHIP brightness
fade, `$8F`), undisbeliever `8a82d25de6e99ae5` / `9cde32ae8100f0c1`, and
anomie-regs `86a167989f25767c` (the `$21FF -> $2100` wrap). `snes_verify`
gave `arbiter_states_point` with the right sentences on both claims.

## Golden queries at this pin: 10 of 10

The nine of `.claude/notes/tech/cartouche_corpus.md` plus "per-window
shift `--max-shift`" (`2dda265e2f4d3aa9`): the intended source first on
each, with `exclude_sources=["opensnes-docs", "opensnes-notes-tech"]`.

## The ask: re-capture luna-docs for v1.39.0

luna released v1.39.0 on 2026-10-18 (`f2a8841`). Its docs are not in the
index yet:

- `luna state --until-pc --until-if expression condition checkpoint until_if`
  returns the v1.36.0 `--until-pc` passage (`77d3ae60bf73ad10`) and the
  `--until-frame` one; nothing on `--until-if`.
- `luna 1.39.0 changelog multiplier divider 5A22 RDMPY RDDIV read too early
  value in progress` returns snesdev-wiki's errata and our withdrawn
  measure; no luna-docs passage. In 1.39.0 luna returns the value in
  progress when `$4214-$4217` is read too early after `$4203`/`$4206`.

Also new in that version and absent: the debugger (`bp_add` with `cpu`,
watchpoints on the coprocessors' memories and the ARAM, `forbid` ranges,
`eval`), `luna record`, `fbhash` in a `[[checkpoint]]`, exact arrival
counts when an interrupt falls on the watched instruction.

Golden query to add once captured: "luna --until-if expression" must
return a luna-docs passage naming `--until-if`.
