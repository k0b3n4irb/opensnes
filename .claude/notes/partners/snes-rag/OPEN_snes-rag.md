# OpenSNES → snes-rag: open items (accumulating, not yet sent)

Opened 2026-09-26. One line per item; every item is re-run against the
corpus the day the report goes out (`.claude/rules/partners.md`).

| Date | Item | Seen on | What we needed |
|---|---|---|---|
| — | (empty: everything sent up to `2026-10-03_to_snes-rag_verify-rejoue-et-v1.31.0.md`; the six `snes_verify` cases were replayed there) | | |
| - | (empty: the delivered index `8bae78f3a746` was verified once the replica was rebuilt, and answered in `2026-10-03_to_snes-rag_citation-et-v1.31.0_reply.md`) | | |
| - | (the half-volume measurement went out in `2026-10-03_to_snes-rag_fil-d-ariane_reply.md`, §3) | | |
| 2026-10-04 | `snes_verify("On an SA-1 cartridge the SNES CPU sees BW-RAM linearly at banks $40-$4F …, and the SA-1 CPU sees it at banks $40-$43")` | `arbiter_covers_topic_only`; the fullsnes memory-map chunk `e4c599f9fd7bdb13` came back with an empty `sentences` list although it states the point ("40h-4Fh:0000h-FFFFh Entire 256Kbyte BW-RAM (mirrors in 44h-4Fh)"); its SA-1-side neighbour `7f0d8665d72049dd` ("Same as on SNES Side … plus 60h-6Fh BW-RAM mapped as 2bit or 4bit pixel buffer") was not returned at all. Meanwhile sneslab-wiki `e409c6eb59bf1180` (complement) says "Mapped on banks $40-$5F on SA-1 CPU side (up to 2 MiB)", which contradicts the arbiter | the two fullsnes chunks as evidence with their sentences, and a documented-error flag (or a contrast note) on the sneslab `$40-$5F` sentence |
| 2026-10-05 | *(suite de la ligne du 10-04)* snes-rag a rejoué le cas (`2026-10-04_from_snes-rag_fin-du-menage.md`, §2) : `sentences` reste vide parce que fullsnes écrit les banques sur deux chiffres (`40h-4Fh`) et que seules les adresses à quatre chiffres sont mises sous forme commune ; **défaut connu, non corrigé**, la lecture d'après fullsnes reste la méthode. Le contraste sneslab `$40-$5F` n'a pas été commenté | `arbiter_covers_topic_only`, `sentences` vide (rejoué par eux) | un alias de forme pour les plages de banques `NNh-MMh` ↔ `$NN-$MM`, ou que `sentences` garde la phrase qui porte les mêmes nombres |

