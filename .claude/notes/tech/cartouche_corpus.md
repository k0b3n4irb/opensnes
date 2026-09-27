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

## Index state (2026-09-27)

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
- `snes_verify(claim, exclude_sources=[…])` — structured verdict:
  `confirmed` / `contradicted` / `unsettled` / `not_covered`, with the
  strongest arbiter citation, documented errors and `chunk_id`s. Always
  compare the claim's wording to the citation: `confirmed` means "an
  arbiter documents this point", not "your sentence is true" (seen
  2026-09-12: a `confirmed` on the cc65816 push order cited a generic
  65c816 stack passage; the real support was `compiler/ABI.md`).

Documented error worth knowing: `qbe-docs` `abi.txt` describes the upstream
targets' ABI (amd64/arm64/rv64); for anything cc65816 / w65816 the arbiter
is `compiler/ABI.md`. The corpus flags this on ABI queries.

## Golden queries (status 2026-09-27, index `c416932c0b34`)

Run with the exclusion set. "✅" = the intended source is in the top 3.

| # | query | status | note |
|---|---|---|---|
| 1 | QBE IL: aggregate type definition syntax `type :name = { w, l, s }` and the phi instruction | ✅ qbe-docs | the looser "how are aggregates declared and passed to a call" phrasing fails — assembler-macro vocabulary (wla-dx, asar) drowns it |
| 2 | QBE IL call instruction with env and variadic marker | ✅ qbe-docs | ✗ until 2026-09-12; green since the authority prior |
| 3 | cproc supported C extensions and C23 features | ✅ cproc-docs | cproc *internals* (struct layout in `type.c`) are not documented anywhere; read the source |
| 4 | luna `--power-on zero ones random seed` uninitialised RAM fill | ✅ luna-docs | also states the manifest keys `power_on` / `seed` |
| 5 | luna test manifest schema; which assert keys exist | ✅ luna-docs | |
| 6 | How does `luna diff --tolerance` match frame F of ROM A to ROM B | ✅ luna-docs | |
| 7 | TMX tile flipping flags (`FLIPPED_HORIZONTALLY_FLAG`…) high bits of the gid | ✗ | still ✗ on 2026-09-27: `tiled-tmx-format` answers with `<tileoffset>` and `<data>`; the "Tile flipping" subsection looks uncaptured — asked again |
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
