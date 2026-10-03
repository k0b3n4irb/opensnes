# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| — | (empty: everything sent up to `2026-10-03_to_snes-rag_verify-rejoue-et-v1.31.0.md`; the six `snes_verify` cases were replayed there) | | |
| - | (empty: the delivered index `8bae78f3a746` was verified once the replica was rebuilt, and answered in `2026-10-03_to_snes-rag_citation-et-v1.31.0_reply.md`) | | |
| 2026-10-03 | A fact we measured that we found in no source (not yet searched in the corpus — search before sending): `snesmodSetModuleVolume(63)` against the default 127 gives a quarter of the output level, not half (RMS per 500 ms window 5491 -> 1329, 4509 -> 1091…; 75.8 % lower, (63/127)^2 = 0.246) | luna `develop` `a47a55e`, `luna diff --audio`, `examples/audio/snesmod_music` at `c4c87e9a` | the corpus carrying it with its provenance, if no source states the volume law |
