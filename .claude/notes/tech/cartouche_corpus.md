---
name: cartouche_corpus
description: What the Cartouche SNES corpus (MCP `cartouche`) contains, its index fingerprint, and the golden queries that prove the toolchain-side sources answer — the in-repo companion of .claude/rules/hardware_claims.md (gaps review item D6).
type: tech
---

# Cartouche corpus — sources, index fingerprint, golden queries

`.claude/rules/hardware_claims.md` mandates `snes_search` (and now
`snes_verify`) for every hardware claim, always with
`exclude_sources=["opensnes-docs", "opensnes-notes-tech"]` so our own docs
cannot answer for the corpus. This note is what the rule does not say:
what is in there, how to tell it moved, and which queries prove the
toolchain-side sources are reachable. Refresh it when `snes_sources`
reports a new index fingerprint.

## Index state (2026-10-02, v8)

`snes_sources`: **34 808 chunks, 210 sources captured of 236, built
2026-10-02T05:26:41Z, chunker v8, index v2, fingerprint `5f0e4bb5e1c0`**
(`2026-10-02_from_snes-rag_ids-v8.md`). **Chunk ids are now derived from
the content** (document, breadcrumb, text) instead of `sha1(document:rank)`,
so every id changed once; old ids still resolve through an alias table in
`snes_get` — 17 of 17 of ours checked, same text. Cite the new id when you
touch a comment. fullsnes's prose (~400 KB) is indexed again. Golden
queries 9/9. Serving it on the replica takes `git pull` + `make import
SRC=/media/psf/Home/workspaces/SNES/snes-rag` + `make rebuild`; the index
is down for the ~2.5 min of the rebuild (reported).

## Index state (2026-10-02, evening)

`snes_sources`: **34 220 chunks, 210 sources captured of 236, built
2026-10-02T03:47:47Z, chunker v7, index v2, fingerprint `0aeced38d56e`**
(`2026-10-02_from_snes-rag_reponse.md`, answering our audit). New: an
address alone no longer attests a point, and a passage listing more than 8
addresses (a register map) attests nothing by its register names — our
multiplier claim now comes out `arbiter_covers_topic_only`, with their
negative-measure note (`mesures-partenaires`) first; `sentences` now also
keep a sentence sharing three terms with the claim (the empty window,
VMADD and Mode 5 sentences are served, state still `topic_only` by design:
the sentence decides); `wladx-issue-704` captured; `mesures-partenaires`
carries our two measures (latch H/V, multiplier non-reproduced); a
`gsu-stop.md` fiche distils ares/bsnes `instructionSTOP`; nesdev forum
threads re-captured whole (~620 → 1 172 chunks). Golden queries: **9/9**;
negative control: our ABI ranks 1-2, never qbe-docs.

## Index state (2026-10-02)

`snes_sources` on 2026-10-02: **33 436 chunks, 209 sources captured of 235,
built 2026-10-02T02:57:24Z, chunker v7, index v2, fingerprint
`31eb1726aa31`** (snes-rag's `2026-09-30_from_snes-rag_bilan-audit.md`,
which supersedes their three earlier notes of the 30th). New: embedding
windows under 512 tokens, query-centred excerpts, a reserved place for
arbiters instead of the ×1.8 multiplier, emulator source code (Mesen2
`Core/SNES`, ares core, sd2snes `cic/`, stuntrace's GSU-2 model — they
answer when the question names their identifiers), a
`mesures-partenaires` source for facts partners measured. Their recall is
now counted on the answering **passage**: 54.1 % at 5 (the source-level
80.7 % was mostly the arbiters' prior). Golden queries rerun: **9/9**;
negative control: our ABI at ranks 1 and 2, never qbe-docs.

## luna pin v1.30.3 (2026-10-02)

Golden queries 4, 5, 6 and 9 (the `luna-docs` ones) rerun on `5f0e4bb5e1c0`:
green. `luna-docs` stops at 1.30.2 (the 1.30.3 Super FX battery entry is not
served); reported in `2026-10-02_to_snes-rag_ids-v8_reply.md` §5.

## Index state (2026-09-30)

`snes_sources` on 2026-09-30: **31 943 chunks, 208 sources captured of 234,
built 2026-09-30T01:35:00Z, chunker v7, fingerprint `55507a6f2907`**
(snes-rag's `2026-09-30_from_snes-rag_c8-luna.md`). What moved:

- **C8 bounded to hardware** (`audited_scope = "hardware"`): the 0.7
  handicap on audited sources stays on every hardware question, drops where
  the `not-toolchain` gate closes. Our negative control (`cc65816 calling
  convention: push order and pointer size`, k=3, no exclusion) answers our
  ABI at ranks 1 and 2 again, `wdc-65816-manual` 3rd; never qbe-docs.
- **`luna-docs` recaptured at v1.30.2** (it had stayed at v1.27.0: luna's
  history rewrite made snes-rag's `git pull --ff-only` fail silently;
  their capture clones now fetch + reset).
- **v0.46.0 in `opensnes-docs`**: the ABI return row we reported is served
  corrected. A chunk id follows the **position** in its document, not the
  content: `913a9c160f2433dd` kept its id with new text — quote ids with
  the date or the fingerprint.
- **`tools.doctor` checks a witness verdict** (our suggestion): sprite Y+1
  must come out `unsettled`, SIWP `confirmed` — the fingerprint proves the
  index, the witness proves the verify code.
- Eval: recall@5 82.5 %, recall@1 55.3 %, recall@10 89.5 %, MRR 0.664.

Golden queries rerun on `55507a6f2907`: **9 of 9 green**, sources as in the
table below.

## Index state (2026-09-27, second update)

Evening (snes-rag's `…_classement.md`, same fingerprint): ranking changed —
`boost_follows` removed, a `consensus_floor` added (when BM25 and the vector
leg both put a passage first, no source weight can bury it), recall@5
78.9 → 83.3 %, recall@1 47.4 → 54.4 %. Three `snes_verify` fixes found by
luna (a mangled `65C816` token, `luna-docs` promoting itself to arbiter on
any topic, a-f words taken for addresses). The 207 captured vs 202 indexed
gap is deduplication: six sources are served under a canonical one (`also_in`
on the chunk). Audit of our citations of the week: none rests on `luna-docs`
as an arbiter (reply2 §0).

Served on 2026-09-27 (`snes_sources`): **31983 chunks, built
2026-09-27T07:50:17Z, chunker v7, fingerprint `bb5dbf5eff5d`**. snes-rag's
report of that morning (`partners/snes-rag/2026-09-27_from_snes-rag_rapport.md`)
announced `c145c7472cf3` (31977, the TMX recapture); the index moved again
after it (their consensus-floor ranking change). Golden queries rerun on
`bb5dbf5eff5d`: 9 of 9 green, no. 7 (TMX flip flags) green for the first
time; the negative control still never answered by qbe-docs.

**`snes_verify` fixed on the service (checked later on 2026-09-27, index
rebuilt 08:02, same fingerprint):** sprite Y + 1 now comes out `unsettled`
on the off-topic 34-slivers passage, and a well-cited claim (SCMR bits,
fullsnes `725e8061d576404e`) still comes out `confirmed`. Read the
citation anyway: a `confirmed` must quote the claim.
The fact itself is arbitrated: snesdev-wiki `857cd9077cef3a88` ("sprites
appear 1 line lower than their Y value … the first line of rendering is
always hidden"), buried at the end of a long OAM chunk, so search does not
return it (their granularity work, gq30).

## Index state (2026-09-27, first update)

`snes_sources` → 207 sources captured of 234 listed; **31973 chunks, 202
sources indexed, built 2026-09-27T06:19:02Z, chunker v7, fingerprint
`c416932c0b34`** (snes-rag's report of 2026-09-26, revised 09-27,
`partners/snes-rag/2026-09-26_from_snes-rag_rapport.md`; checked with
`snes_sources` the same day).

**What went wrong on 2026-09-26, and what the fingerprint means now.** The
instance we query is snes-rag's service VM; it had pulled the new ranking
policy by git but not the chunks (`make import`), so it served the corpus
of 09-12 under a chunker label of 09-25. Two metadata bugs hid it, both
fixed by snes-rag: `chunk_version` described the code, not the chunks (it
now reads the ledger and says `6+7` on a mixed index), and the fingerprint
covered chunk ids only (it now covers each chunk's content hash). So the
fingerprint is again the trigger: **when it moves, rerun the golden
queries.** Compare the indexed-source count (202) with `snes_sources`'
captured count as a second check.

Also new since 2026-09-25: an **authority prior** (`arbiter_boost = 1.80`
on the seven general arbiters, not applied to toolchain vocabulary, so our
docs still win on ABI / luna / asset formats; `audited_weight = 0.7` for
opensnes-docs unchanged); `ultrastarfox` is indexed but down-weighted on
purpose — its distilled fiche answers, the source is there to be checked.

Previous states: 2026-09-25 (served) 30848 chunks, fingerprint
`e2a738a553ec`, chunker v7 label on v6 chunks; 2026-09-12: 201 of 223,
30848 chunks, chunker v6, `e2a738a553ec`.

Reachable since 2026-09-27 (our four reproduction queries of 09-26 replayed):
domain arbiter **sd2snes-changelog**; solid `ultrastarfox` (via the fiche),
`argsfx-sasm-docs`; complement `peterlemon-gsu`, `cartouche-fiches`,
`cartouche-fiches-jeux`. `luna-docs` re-captured at v1.27.0.

**`snes_verify` caveat (2026-09-27):** it can answer `confirmed` with a
citation that does not state the claim (sprite Y + 1: cited the 34-slivers
passage). Read the citation before trusting the verdict; reported.

General arbiters: snesdev-wiki, fullsnes, anomie-regs, anomie-timing,
anomie-sdsp, anomie-spc700, undisbeliever-snesdev.

Domain arbiters: mame-upd7725, ares-coprocessors, higan-boards-bml,
doom-fx-source, blargg-snes-spc, gilyon-snes-tests, 6502org-816-opcodes,
pandocs-sgb, retroreversing-gigaleak, and since 2026-09-12 the toolchain
set requested by the gaps review: **qbe-docs**, **cproc-docs**,
**luna-docs**, **tiled-tmx-format**, **aseprite-file-spec** (plus
`snes-sdk-hecht`, solid).

Ours, indexed: `opensnes-docs`, `opensnes-notes-tech` (recaptured 2026-09-24). They are
the reason for the mandatory exclusion.

Still not captured (2026-09-12): Calypsi / WDC816CC / vbcc 65816 manuals
(asked; may be a licensing matter), `llvm-mos` (watch),
`console5-snes-schematics`, `smwcentral-beginners-guide` (to-capture).

## The two tools

- `snes_search(question, exclude_sources=[…], k, authority_min, contrast)` —
  passages with authority labels and documented-error warnings.
  `contrast=true` for disputed points (SIWP-class).
- `snes_verify(claim, exclude_sources=[…])` — since 2026-09-30 it serves
  `evidence_state` (`not_covered` · `no_arbiter` ·
  `arbiter_covers_topic_only` · `arbiter_states_point` ·
  `documented_error_on_point`), `limits`, and `evidence`: up to five
  passages with their `sentences`. Our rule (`hardware_claims.md`)
  requires `arbiter_states_point` **and** a sentence that states the
  polarity or value; the historical `verdict` is not read. Both
  directions fail sometimes (2026-10-02: a false `arbiter_states_point` on
  the multiplier during auto-joypad, false `topic_only` on the empty
  window and on VMADD incrementing) — the sentences decide.

Documented error worth knowing: `qbe-docs` `abi.txt` describes the upstream
targets' ABI (amd64/arm64/rv64); for anything cc65816 / w65816 the arbiter
is `compiler/ABI.md`. The corpus flags this on ABI queries.

## Golden queries (status 2026-10-02, index `5f0e4bb5e1c0`; 9/9 on every index since `bb5dbf5eff5d`)

Run with the exclusion set. "✅" = the intended source is in the top 3.

| # | query | status | note |
|---|---|---|---|
| 1 | QBE IL: aggregate type definition syntax `type :name = { w, l, s }` and the phi instruction | ✅ qbe-docs | the looser "how are aggregates declared and passed to a call" phrasing fails — assembler-macro vocabulary (wla-dx, asar) drowns it |
| 2 | QBE IL call instruction with env and variadic marker | ✅ qbe-docs | ✗ until 2026-09-12; green since the authority prior |
| 3 | cproc supported C extensions and C23 features | ✅ cproc-docs | cproc *internals* (struct layout in `type.c`) are not documented anywhere; read the source |
| 4 | luna `--power-on zero ones random seed` uninitialised RAM fill | ✅ luna-docs | also states the manifest keys `power_on` / `seed` |
| 5 | luna test manifest schema; which assert keys exist | ✅ luna-docs | |
| 6 | How does `luna diff --tolerance` match frame F of ROM A to ROM B | ✅ luna-docs | |
| 7 | TMX tile flipping flags (`FLIPPED_HORIZONTALLY_FLAG`…) high bits of the gid | ✅ tiled-tmx-format | green since `bb5dbf5eff5d`: the constants live on the "Global Tile IDs" page, which the corpus now captures (`d07b82b0b0dcebb9`, `8a64c4c6f6f9932f`) |
| 8 | Aseprite file format: cel chunk layout and palette chunk semantics | ✅ aseprite-file-spec | |
| 9 | luna profile per-symbol master cycles `--from-frame --top` JSON | ✅ luna-docs | |
| N | *negative control* — cc65816 calling convention: push order and pointer size (no exclusion) | ≈ opensnes-docs 2nd and 3rd | 2026-09-27: `wdc-65816-manual` "Push" (generic) is first; never qbe-docs, so the control holds, but our ABI is no longer first — reported. The chunk it served carried a stale row of our own ABI.md (fixed the same day) |

Lesson from the first run: phrase toolchain queries with the tool's own
vocabulary (`type :name`, `phi`, `--power-on`, `cel chunk`), not with
generic compiler words; the lexical side of the ranking is strong.

## Cross-references

- `.claude/rules/hardware_claims.md` — the rule this note serves.
- `.claude/notes/chantiers/hardware_docs_audit.md` — the 2026-09 audit
  whose golden queries lived outside the repo until this note.
- `.claude/notes/partners/snes-rag/` — the request and validation reports
  sent to the corpus team (2026-09-11, 2026-09-12; moved into the repo from
  the owner's machine on 2026-09-22) and `OPEN_snes-rag.md`, the report
  being accumulated for the next send. The contract with the corpus team
  is `.claude/rules/partners.md`.
