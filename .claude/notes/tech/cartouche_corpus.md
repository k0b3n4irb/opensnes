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

## Index state (2026-09-12)

`snes_sources` → 201 sources captured of 223 listed; **30848 chunks, built
2026-09-12T09:12:20Z, chunker v6, fingerprint `e2a738a553ec`**. A different
fingerprint means the corpus changed: rerun the golden queries below.

General arbiters: snesdev-wiki, fullsnes, anomie-regs, anomie-timing,
anomie-sdsp, anomie-spc700, undisbeliever-snesdev.

Domain arbiters: mame-upd7725, ares-coprocessors, higan-boards-bml,
doom-fx-source, blargg-snes-spc, gilyon-snes-tests, 6502org-816-opcodes,
pandocs-sgb, retroreversing-gigaleak, and since 2026-09-12 the toolchain
set requested by the gaps review: **qbe-docs**, **cproc-docs**,
**luna-docs**, **tiled-tmx-format**, **aseprite-file-spec** (plus
`snes-sdk-hecht`, solid).

Ours, indexed: `opensnes-docs`, `opensnes-notes-tech` (2026-09-03). They are
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

## Golden queries (status 2026-09-12)

Run with the exclusion set. "✅" = the intended source is in the top 3.

| # | query | status | note |
|---|---|---|---|
| 1 | QBE IL: aggregate type definition syntax `type :name = { w, l, s }` and the phi instruction | ✅ qbe-docs | the looser "how are aggregates declared and passed to a call" phrasing fails — assembler-macro vocabulary (wla-dx, asar) drowns it |
| 2 | QBE IL call instruction with env and variadic marker | ✗ | same collision; reported to the RAG team (retrieval, not ingestion) |
| 3 | cproc supported C extensions and C23 features | ✅ cproc-docs | cproc *internals* (struct layout in `type.c`) are not documented anywhere; read the source |
| 4 | luna `--power-on zero ones random seed` uninitialised RAM fill | ✅ luna-docs | also states the manifest keys `power_on` / `seed` |
| 5 | luna test manifest schema; which assert keys exist | ✅ luna-docs | |
| 6 | How does `luna diff --tolerance` match frame F of ROM A to ROM B | ✅ luna-docs | |
| 7 | TMX tile flipping flags (`FLIPPED_HORIZONTALLY_FLAG`…) high bits of the gid | ✗ | `tiled-tmx-format` answers other sections; the "Tile flipping" subsection may be uncaptured — asked |
| 8 | Aseprite file format: cel chunk layout and palette chunk semantics | ✅ aseprite-file-spec | |
| 9 | luna profile per-symbol master cycles `--from-frame --top` JSON | ✅ luna-docs | |
| N | *negative control* — cc65816 calling convention: push order and pointer size (no exclusion) | ✅ opensnes-docs first | must never be answered by qbe-docs |

Lesson from the first run: phrase toolchain queries with the tool's own
vocabulary (`type :name`, `phi`, `--power-on`, `cel chunk`), not with
generic compiler words; the lexical side of the ranking is strong.

## Cross-references

- `.claude/rules/hardware_claims.md` — the rule this note serves.
- `.claude/notes/chantiers/hardware_docs_audit.md` — the 2026-09 audit
  whose golden queries lived outside the repo until this note.
- `~/opensnes_reports/opensnes_report_snes-rag_2026-09-1{1,2}*.md` — the
  request and validation reports sent to the corpus team (owner's machine).
