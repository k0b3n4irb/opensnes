# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| 2026-09-26 | **Sprite Y is displayed one line lower than OAM Y (OAM Y = N draws from scanline N+1)** — our lib compensates (`oamSet` stores y − 1) and we needed an arbiter while fixing the dynamic sprite engine, which did not. Two queries, `exclude_sources=[opensnes-docs, opensnes-notes-tech]`: "OAM sprite Y coordinate: a sprite with Y = N is displayed starting on which scanline (N or N+1)?" and, `contrast=true`, "sprites appear one line lower than their OAM Y value…" — both return the 32-sprites / 34-slivers passages (snesdev-wiki `f09ef25c4bbb2d5f`, `86925d3a676f452b`, anomie-regs `f7a410615192bf28`), none states the offset. | a passage that states it (fullsnes and anomie discuss sprite Y timing), or the corpus saying it is unsettled |
