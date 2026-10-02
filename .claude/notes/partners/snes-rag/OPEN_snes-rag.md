# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| 2026-10-02 | Evidence for the known false-`arbiter_covers_topic_only` shape, still there under v8 (`5f0e4bb5e1c0`): `snes_verify("In Mode 4 the offset-per-tile table has a single row; bit 15 of an entry selects vertical offset, and the low 3 bits of a horizontal offset are ignored.", exclude_sources=[opensnes-docs, opensnes-notes-tech])` → `unsettled` / `arbiter_covers_topic_only`, while the sentences of `d594aeedde1b87c2` state all three points ("Mode 4 only reads 1 row", "the bottom three bits are ignored", "Scroll direction (0 = horizontal, 1 = vertical)") | `snes_verify` | `arbiter_states_point` when an arbiter sentence states the claim |
| 2026-10-02 | Pseudo-hires `contradicted` (already sent) re-run under v8: none of the five `evidence[].sentences` states which screen takes the even columns, although the `documented_errors` entry itself says sub = even, i.e. agrees with the claim | `snes_verify` | the arbiter sentence that states the column order (snesdev-wiki *PPU registers*, anomie-regs, fullsnes) in `evidence` |
