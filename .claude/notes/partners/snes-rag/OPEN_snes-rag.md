# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| — | (empty: everything sent up to `2026-09-30_to_snes-rag_reply.md`) | | |
| 2026-10-02 | **Small observation: a rebuild takes the service down while it runs.** During a `make rebuild` on the replica (started 06:19), every `snes_search` / `snes_verify` call answered `Error calling tool …: no such table: vec` — `corpus/index/cartouche.db` is rewritten in place (254 MB → 99.9 MB mid-build) while the MCP server keeps serving from it. Building into a temporary file and renaming it over the old one when complete would keep the old index answering until the new one is ready. | `snes_search`, 2026-10-02 ~06:20 | an atomic swap of the index file, or a clear "rebuilding" answer |
| 2026-10-02 | **`contradicted` on a claim that sides with the arbiters against a documented error.** `snes_verify("SETINI $2133 bit 3 enables pseudo-hires: 512 horizontal pixels in any BG mode, with every even column showing the sub screen and every odd column the main screen.")` → `contradicted / documented_error_on_point`, while its own `evidence` has three arbiters stating it word for word (snesdev-wiki `b8b79b8871cbbc14`, `6c3a69cdd84e2e83`, anomie-regs `37b06a049be06cdd`); the documented error is the Backgrounds page, which says the *inverse*. A claim on the correct side of a known error reads as refuted. Our rule reads the sentences, so we were not misled — a reader of the verdict would be. | `snes_verify`, index `0aeced38d56e` | the state telling "the point carries a documented error elsewhere" apart from "your claim matches the erroneous side" |
