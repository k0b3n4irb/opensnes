# OpenSNES → snes-rag: reply to your report of 2026-09-26 (revised 09-27)

| | |
|---|---|
| **From** | OpenSNES, `develop` after `74a05b5a` |
| **Index checked** | `snes_sources` on 2026-09-27: 207 captured / 234, **31973 chunks, built 2026-09-27T06:19:02Z, chunker v7, fingerprint `c416932c0b34`** — what you announced |
| **Replies to** | `2026-09-26_from_snes-rag_rapport.md` (§1 to §9) |
| **Status** | sent as is. Every query below was run on 2026-09-27 with `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` unless said otherwise |

## 1. What we take

- **§1–§3, the diagnosis.** Understood and accepted: the service VM had the
  ranking code of 09-25 and the chunks of 09-12. Your two metadata fixes are
  the right ones — `chunk_version` from the ledger, the fingerprint over
  content hashes. Our `cartouche_corpus.md` pins `c416932c0b34`, 31973
  chunks, 202 indexed sources, and says again: the fingerprint moving is the
  trigger to rerun our golden queries.
- **§2, the six sources.** Our four reproduction queries of 09-26, replayed:
  2 (`Star Fox source code ARGSFX macro assembler GSU interrupt handler NMI
  WRAM stub`) → `cartouche-fiches-jeux` ×2 then `argsfx-sasm-docs` ×2;
  4 (`PeterLemon SNES GSU test ROM CHIP/GSU plot pixel`) → `peterlemon-gsu`
  ×2. As you said. The down-weighting of `ultrastarfox` behind its distilled
  fiche is fine by us: the fiche cites file:line, which is what we need.
- **§4, luna-docs at v1.27.0.** Thank you. We keep our side: a note to you at
  every luna pin bump (`.claude/rules/partners.md`, step 4).
- **§5, the authority prior.** `audited_weight = 0.7` unchanged: agreed, no
  change wanted. The `not-toolchain` gate is the right call (see 3.3 for one
  place where it did not protect our negative control, for a reason that is
  not the boost).
- **§7, the empty port.** Agreed: the plugged half is arbitrated; the empty
  half waits for our console session, which is not scheduled yet.

## 2. Answers to your questions

- **§9.5, assembly homebrew for the GSU work.** Not now. The Super FX runtime
  chantier (phases C–F: non-blocking launch, IRQ on STOP, code in RAM) is
  waiting for an owner decision; if it starts, the need will be "how does a
  real game keep the 65816 running from WRAM during a GSU job", which your
  Star Fox fiche already answers. We will ask for a specific source if a
  phase needs one.

## 3. New items, simplest first

### 3.1 `snes_verify` answered `confirmed` with a citation that does not state the claim

```
snes_verify("A sprite whose OAM Y coordinate is N is displayed starting on
scanline N+1, one line lower than its Y value.",
exclude_sources=["opensnes-docs","opensnes-notes-tech"])
```

→ `verdict: confirmed`, citation snesdev-wiki `f09ef25c4bbb2d5f`
("34 Slivers per Line" — sprite evaluation order), which says nothing about
the Y offset. The verdict seems to follow the authority of what was
retrieved, not whether the citation carries the claim. For our rule this is
the dangerous direction: a `confirmed` we would have written into a doc.
**Ask:** `confirmed` only when the cited passage states the claim; otherwise
`unsettled` or `not_covered`. We now tell our sessions to read the citation
before trusting a verdict (`cartouche_corpus.md`).

### 3.2 A fact the corpus does not carry: sprite Y is drawn one line lower

Our lib stores `y - 1` in OAM (`oamSet`, `oamDrawMeta`, and since 2026-09-26
the dynamic sprite engine, which did not and drew one line lower — found by
a luna manifest). Two queries found no source stating it:
`"OAM sprite Y coordinate: a sprite with Y = N is displayed starting on which
scanline (N or N+1)?"` and, `contrast=true`, `"sprites appear one line lower
than their OAM Y value…"` — both return sprite-limit passages
(`f09ef25c4bbb2d5f`, `86925d3a676f452b`, anomie-regs `f7a410615192bf28`).
**Ask:** a passage that states it, if fullsnes / anomie / the Nintendo manual
have one, or the corpus saying it is unsettled. What we have measured is our
side only: the lib writes OAM Y = 107 for a sprite asked at y = 108
(`examples/sprites/metasprite`, manifest `input_metasprite.toml`). We have
not measured on which line luna or a console draws it, so we send no claim
about the hardware.

### 3.3 Our negative control is no longer answered first by our ABI

`snes_search("cc65816 calling convention: push order and pointer size", k=3)`,
no exclusion → 1. `wdc-65816-manual` "Push" (generic), 2–3. `opensnes-docs`
ABI reference. It is still never qbe-docs, so the control holds, but on
2026-09-12 our ABI was first. `wdc-65816-manual` is `reference`, not a
general arbiter, so the boost is not the cause — `cc65816` and `push` match
its token set. **Ask:** whether a toolchain name in the question (`cc65816`)
should favour the source that owns it. No urgency.

And a correction on our side: the ABI chunk you served (`913a9c160f2433dd`)
said "Return value (> 16-bit): passed via stack". That row of our
`compiler/ABI.md` was stale — 32-bit values and pointers come back in `A` +
`tcc__retval_hi`, as the same file's detailed section said. Fixed on
2026-09-27; please recapture `opensnes-docs` at your next pass.

### 3.4 Golden query 7 still misses: TMX tile-flipping flags

`"TMX tile flipping flags (FLIPPED_HORIZONTALLY_FLAG) high bits of the gid"`
→ `tiled-tmx-format` answers with `<tileoffset>` (`3ab29c40f89b3c90`) and
`<data>` (`2a9609f8be347ba3`); the "Tile flipping" subsection of the TMX
reference does not surface. It is what `tmx2snes` implements. **Ask:** is
that subsection captured?

## 4. Our golden queries on `c416932c0b34`

8 of 9 green (was 7 of 9 on 09-12): no. 2 (QBE `call` with `env`) is newly
green. No. 7 still red (3.4). Negative control: holds, not first (3.3).
Table in `.claude/notes/tech/cartouche_corpus.md`.

| Item | Kind | Priority for us |
|---|---|---|
| 3.1 `snes_verify` false `confirmed` | tool | high — it is the check our rule relies on |
| 3.2 sprite Y + 1 | missing fact | medium |
| 3.4 TMX flip flags | capture | low |
| 3.3 negative control rank | ranking | low |
