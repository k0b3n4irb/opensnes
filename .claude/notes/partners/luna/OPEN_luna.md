# OpenSNES → luna: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item. Every item is re-checked on the pinned
luna the day the report goes out (`.claude/rules/partners.md`); the pin is
v1.30.4 (2026-10-03).

| Date | Item | Seen on | What we would run |
|---|---|---|---|
| 2026-10-02 | `luna test` has no region key: a manifest cannot run its ROM as the other video standard (the CLI has `--force-region`, `luna test --help` names no region). For the PAL pass on the games, `make test-pal` builds each game a second time with header `$FFD9 = $02` and rewrites the manifest's `rom`; it cannot replay an NTSC-declared ROM on a PAL console (an import cart on a PAL machine) without a rebuild. Low priority, the rebuild route works. | v1.30.3 | `region = "pal"` beside `power_on`, echoed in `--report json` like the power-on pair; we would add it to copies of `state_tetris`, `movement_breakout`, … and drop the PAL rebuild from `make test-pal` |
