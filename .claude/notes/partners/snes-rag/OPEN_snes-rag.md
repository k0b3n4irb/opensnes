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
| 2026-10-05 | `snes_search("sd2snes firmware changelog: Super FX (GSU) support added, which version", k=6)`: the domain arbiter `sd2snes-changelog` surfaces only its v1.10.0 chunk (`99d8550526ffa29f`, S-DD1); the Super FX / SA-1 support line of the changelog does not come up on this or the chips audit's two queries, and our `HARDWARE_VERIFICATION.md` leans on the blog (`c94f64959972104e`, 1.10.3 notes; `bb618727c8492d28`, 1.11.1) | arbiter present, point not served | the changelog's own line for SA-1 / Super FX cores (firmware 1.10.x), so the fact rests on the arbiter and not on the blog |
| 2026-10-05 | How the FXPak / sd2snes chooses `dsp1.bin` vs `dsp1b.bin` for a homebrew DSP-1 ROM is stated nowhere in the corpus (the downloads page lists both files, `0f24744ff001c043`); a `Distance` difference in a console session could not be attributed without it | not covered | any source stating the selection rule (header? both tried?), or a note that none exists |
| 2026-10-05 | `snes_verify("On the Super FX, the CFGR MS0 bit (fast multiplier) must not be set when the GSU runs at 21 MHz (CLSR = 1)…")` → `arbiter_covers_topic_only` while the fullsnes sentence served states it word for word (« MS0 <must> be zero in 21MHz mode », `1adef8e33ff3c4e9`, `states_point: false`) — the second failure shape of 2026-10-02 again; the claim went into `superfx.h` on the sentence | `topic_only` with a stating sentence | `states_point` true when the sentence carries the claim's register and condition |
| 2026-10-09 | Good news, and a correction of ours. On 2026-09-26 two queries about the scanline a sprite's OAM Y lands on "did not settle" for us and were logged here. Today `snes_search("OAM sprite Y coordinate: on which scanline does a sprite with Y=0 start, sprites appear one line lower than their Y value, first visible scanline and background VOFS line 0 not displayed", exclude_sources=[opensnes-docs, opensnes-notes-tech], k=8)` returns the arbiter sentence at rank 1 (snesdev-wiki, `9dd075095fd0d965`): "a sprite with Y=0 will appear to begin on the first visible line. However, a background with Y scroll of 0 will appear to have its top pixel cut off by the hidden line". Our library had stored y - 1 for sprites since 2026-04-27 on the first half of that sentence, citing chunk `857cd9077cef3a88` (an id today's result does not carry); fixed today (sprites stored as given), measured on luna: OAM Y = 0 covers picture lines 0-7, Y = 255 lines 0-6. Nothing asked: the corpus was right and we had read half a sentence. If `857cd9077cef3a88` was the same passage before a re-chunking, a way to resolve a retired chunk id to its successor would have shown it |

- 2026-10-10 — owed by us after `2026-10-05_from_snes-rag_bwram-sa1.md`:
  rewrite `lib/include/snes/sa1.h` line 17 ("banks $40-$4F on both CPUs")
  as SNES side `$40-$4F`, SA-1 side `$40-$5F`, with the fiche's two
  reserves and its chunk ids. Next batch that touches headers.
- 2026-10-10 — relayed, not re-queried: the speedball2 project found no
  arbiter for the SA-1's effective speed by memory (its decision 0007).

**2026-10-10, pin v1.37.0: `luna-docs` lags the tag.** Query (no
exclusion): "luna diff --audio --align-onset per-window shift --max-shift".
Returned: `86a0d1666e161484` and `f2ca4b411354b69e` (luna-docs,
arbitre-domaine), which describe `--align-onset` as it was in v1.35.0 /
v1.36.0 — one shift, taken at each capture's first sample above
`--silence`. v1.37.0 (luna `f44025b`, 2026-10-10) fits every window by
itself within `--max-shift` samples (default 64) and its final line says
"per-window shift, max N samples (searched ±64)". Nothing in the corpus
names `--max-shift`. Needed: `luna-docs` re-captured at v1.37.0. Also new
in that tag and absent for the same reason: `--input-at`, `--poke-at` /
`[[poke]]`, `test --update` printing `UPDATED`, and the duration of a DMA
(the end of a burst realigned on the whole burst's count, as ares, Mesen2
and anomie's timing document have it: ±2 or 4 master cycles from a
6-cycle access) — that last one is a hardware fact with three sources
luna cites, worth a fiche of its own.

Sent the same evening in `2026-10-10_to_snes-rag_vitesse-sa1-et-index_reply.md`
(delivered in their folder), with the SA-1 corrections their note asked
for: `sa1.h` and `docs/tutorials/sa1.md` now follow the console photographs
(5.04 MHz for ROM / ROM, not 5.4) and give BW-RAM as `$40-$4F` from the
main CPU, `$40-$5F` from the SA-1 with the fiche's reserve. The line above
that owed the `sa1.h` rewrite is paid.


**2026-10-10, later: re-queried after the owner said the corpus was
updated** (MCP reconnected in this session). Same query, no exclusion,
"luna diff --audio --align-onset per-window shift --max-shift": still
`86a0d1666e161484` and `f2ca4b411354b69e`, still the single-shift text of
v1.35.0 / v1.36.0 ("starts each capture's windows at its own first sample
above `--silence`"); nothing names `--max-shift`. So as served here,
`luna-docs` is not at v1.37.0 yet — or this update was about something
else. On the hardware fact we proposed a fiche for, the corpus already
answers with an arbiter: "DMA duration… realigns to a whole number of its
own cycles…" gives anomie-timing, *S-CPU (5A22) / DMA*
(`a8f6e03510109a8f`): "after the pause, wait 2-8 master cycles to reach a
whole multiple of 8 master cycles since reset". The fiche is therefore not
needed for the fact itself; what no source states is which emulators
followed it and since when, which is luna's to report.

**2026-10-10, third query, after the MCP itself was updated: closed.** The
same query now returns `2dda265e2f4d3aa9` (luna-docs, arbitre-domaine):
"`--align-onset` compares each window of A with the stretch of B that
fits it best … within `--max-shift` samples either way, and prints the
shift it kept", with the `echo` example and its `shift=+2` lines. That is
v1.37.0. The request of `2026-10-10_to_snes-rag_apres-mise-a-jour-luna-docs.md`
(§2) is answered; what we saw before was our own connection serving the
earlier index, as that report had allowed for. Golden query to add at each
luna pin: "per-window shift --max-shift" must return a passage that names
`--max-shift`.
