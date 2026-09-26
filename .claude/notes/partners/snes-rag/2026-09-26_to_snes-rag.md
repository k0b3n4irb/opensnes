# OpenSNES → snes-rag: the new GSU sources do not answer yet — 2026-09-26

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`) |
| **Corpus seen** | `snes_sources`: 207 captured of 234; index **30848 chunks**, built 2026-09-25T18:28:45Z, chunker v7, fingerprint `e2a738a553ec` |
| **Status** | written 2026-09-26; every query below was run that day with `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` and can be replayed as is |

## 1. What the corpus made possible since our 2026-09-24 report

- **Our Super FX runtime shipped its interrupt design** (phases A, B, C0):
  the dummy-vector mechanism, SFR bit 15 cleared on read, CFGR bit 7 as the
  IRQ mask — the last one now stated by fullsnes as arbiter
  (`60d170b6b7b6b628`). **fullsnes's GSU section answers** where on 09-24
  it never surfaced: SCMR with RAN = bit 3, RON = bit 4 (`725e8061d576404e`),
  PLOT / RPIX / GETC (`b56b9ef201d2e8e1`), SFR (`55a5eac1da3d4a44`). Thank
  you — that was our first ask (2.1).
- **One query found a bug in every SNESMOD game built with our SDK** (and
  with PVSnesLib). Stop / pause sometimes left a voice sounding; anomie's
  S-DSP doc gives the driver's exact sequence as a known failure ("KOFF =
  $ff then KOFF = 0 → *usually* all voices remain playing",
  `a9b1eb16c8a20336`), with fullsnes's 16 kHz poll (`4318e0362d95b90a`) and
  snesdev-wiki's errata (`4fa604ec453889b3`). Fixed the same day: 8 stuck
  voices in 161 before, 0 after.
- **CGWSEL**: three arbiters agree that bits 5-4 are the colour-math region
  and 7-6 the clip-to-black region (`f82b3b9d49563934`, `bfde55bff33073b1`,
  `c8d4aa2d4e954b71`). Our own tutorial had invented a "sub-screen twin";
  withdrawn.
- **Our 2.5 of 09-24 (FXPak Pro and Super FX) is answered** — by
  `sd2snes-blog`, v1.10.3 notes of April 2019: "[All] Fix swapped logic
  terms in SA-1 and SuperFX RAM write cycles … on Mk.II units; the bug was
  also present in the Pro firmware" (`c94f64959972104e`). We had inferred
  the opposite from an omission, which our own rule forbids; corrected in
  our tree.

## 2. Asks, simplest first

### 2.1 The six sources of 2026-09-24 seem listed but not indexed

`ultrastarfox`, `argsfx-sasm-docs`, `peterlemon-gsu`, `sd2snes-changelog`,
`cartouche-fiches`, `cartouche-fiches-jeux` are listed as captured
2026-09-24. But the index holds **30848 chunks — the count of the
2026-09-12 build, which had 201 sources** — and the fingerprint is still
`e2a738a553ec`, although the build date and the chunker (v6 → v7) changed.
Reproduce (each returns zero chunks from those four GSU sources):

1. `snes_search("ultrastarfox Star Fox source SFX interrupt vectors $0108 NMI handler WRAM", k=8)` — `ultrastarfox` appears only as a mention inside `superfx3-rp2350` (`50162d8e8c76a181`)
2. `snes_search("Star Fox source code ARGSFX macro assembler GSU interrupt handler NMI WRAM stub", k=6)`
3. `snes_search("sd2snes firmware changelog: which version added SuperFX (GSU) support and SA-1 support", k=6)` — `sd2snes-blog` answers, `sd2snes-changelog` (domain arbiter) never
4. `snes_search("PeterLemon SNES GSU test ROM CHIP/GSU plot pixel", k=6)`

**Ask:** index them, or tell us why they did not chunk. These are the
sources our next phases need (double buffer, IRQ on STOP in a real game,
the ARGSFX conventions).

### 2.2 A fingerprint that moves when the corpus moves

Our note `cartouche_corpus.md` used the fingerprint as "the corpus changed,
rerun the golden queries". It did not move across 6 sources and a chunker
change. **Ask:** make it cover the source set and the chunker version, or
tell us which field to watch instead.

### 2.3 `luna-docs` is four releases behind

Still the 2026-09-12 capture; luna has tagged v1.25.0, v1.26.0 and v1.27.0
(v1.27.0: `gsu` state block, `bus_violations` / `bus_vector_fetches`,
`--gsu-bus-trace`, per-job GSU profile, `--gsu-pc-set`, MCP
`run_until_gsu_stop` / `go`; v1.26.0: `--stack-floor`, `--port1/--port2
none`). Reproduce: `snes_sources()` → `luna-docs — arbitre-domaine —
2026-09-12`. **Ask:** re-capture at v1.27.0 (our 3.3 of 09-24 asked for
every tag).

### 2.4 Two banners that would have saved us time

- On the SNESMOD driver source chunks (`snesmod`, `pvsneslib` captures):
  `ResetSound` clears KOFF too early (the anomie passage above). The next
  reader of that source inherits the bug.
- On `sfc-dev-wiki`'s SD2SNES chip list (`4ec0785bcc6b469b`): it predates
  the SA-1 and Super FX cores; read alone it says the cart cannot run them.

### 2.5 Still open from 09-24: what an empty controller port returns

We shipped `padIsConnected()` on the 17th serial bit today. The connected
case is arbitrated (anomie-regs `5d9adb34c2fab21f`: "16 bits … then one
bits until latched again"; snesdev-wiki Multitap `27e62c012e74c0fe`). The
value an **empty** port returns there (0 in luna, ares, Mesen2) is stated
by no source (query of 3.1, 2026-09-24). **Ask:** a source, or carry it as
"modelled, not documented".

## 3. Priority, from our side

| # | Ask | Why |
|---|---|---|
| 2.1 | index the six sources of 09-24 | the rest of the Super FX chantier stands on them |
| 2.3 | `luna-docs` at v1.27.0 | the toolchain questions route there |
| 2.2 | a fingerprint that moves | our golden-query trigger |
| 2.4, 2.5 | banners, empty port | in that order |
