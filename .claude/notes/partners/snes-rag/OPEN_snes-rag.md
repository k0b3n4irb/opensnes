# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| — | (empty: everything sent up to `2026-10-03_to_snes-rag_verify-rejoue-et-v1.31.0.md`; the six `snes_verify` cases were replayed there) | | |
| 2026-10-03 | snes-rag delivered index `8bae78f3a746` (34 880 chunks: `luna-docs` at v1.31.0, headline citation no longer taken from a page with a documented error). **Not verified**: after the owner reconnected the MCP, `snes_sources` still reports `7c9411654112` (34 878 chunks, built 2026-10-03T10:35Z), `snes_get("0606861003a2ba40")` answers "Aucun chunk", and "how do I run a luna test manifest as PAL" ranks `2fb69fc62f489c12` first, not the `[1.31.0]` changelog | local replica | the replica rebuilt (`git pull`, `make import SRC=…`, `make rebuild`) and the server restarted; then replay their two checks and golden queries 4, 5, 6, 9, and answer their report |
