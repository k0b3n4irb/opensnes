# OpenSNES → snes-rag : open report (accumulating since 2026-09-22)

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`, post-v0.44.0) |
| **Corpus seen** | 201 of 223 sources, 30848 chunks, fingerprint `e2a738a553ec` (built 2026-09-12) — unchanged since our last validation report |
| **Status** | accumulating; every claim re-checked against the corpus on 2026-09-22 with the query and chunk ids given, so it can be sent as is (`.claude/rules/partners.md`) |

## 1. What the corpus made possible since the 2026-09-12 validation

- **Every hardware claim of the v0.44.0 cycle was arbitrated before it was
  written**, and the corpus answered first time on the ones that mattered:
  `$4017` bits 2-4 tied high / 5-7 open bus (fullsnes + snesdev-wiki,
  both arbiters — this is what let us write the `padIsConnected()` caveat
  with a citation instead of "luna says"); HiROM battery RAM at
  `$30-$3F:6000` (fullsnes), which settled the SRAM address bug.
- The `exclude_sources` discipline paid off in the other direction too: on
  a claim our own docs had wrong (the "coordinates must live in an s16
  struct" doctrine), the corpus could not confirm it, and that was the
  right answer.

## 2. Requests, simplest first

### S1 — `luna-docs` lags luna by three releases

**Kind:** re-capture. **Cost:** small (the source exists; it is a refresh).

`luna-docs` was captured 2026-09-12. luna pinned 1.24.0 on 2026-09-17,
tagged 1.25.0 on 2026-09-19, and 1.26.0 is queued with the things we asked
for: `--port1 none` / `--port2 none`, `luna profile --stack-floor` and
`cpu.sp_min`, input flags on `profile`, the `$4016/$4017` open-bus model.
None of that is queryable today, so golden queries 4-6 and 9 of
`cartouche_corpus.md` answer for an older tool.

Reproduce: `snes_search("luna profile --stack-floor sp_min deepest stack
pointer gate exit code", exclude_sources=[opensnes-docs, opensnes-notes-tech])`
→ two WDC-manual fragments and one `luna-docs` chunk (`33ab516edca6fb25`)
that shows `--from-frame --until-frame --top`, no `--stack-floor`
(2026-09-22).

**Ask:** re-capture `luna-docs` (README, docs, CHANGELOG) at every luna
tag. luna can tell you directly when it tags — we have asked them to, and
we will re-run the golden queries at each pin bump and report if the
capture lags.

### S2 — what an unplugged controller port reads: no source states it

**Kind:** a fact the corpus does not carry. **Cost:** editorial.

We needed to know how to tell an empty port from an idle pad. We queried
fullsnes ("Controllers I/O Ports — Automatic Reading"), anomie-timing,
snesdev-wiki and the Super Famicom Dev Wiki: all give the standard pad's
4-bit signature (`0000`), none states what an empty port returns on
auto-read or on manual clocking past bit 16. luna, ares and Mesen2 agree
on a model — auto-read `$0000` for both; past bit 16 a pad's data line
idles high (1s for ever) and an empty port returns 0s — but that is an
emulator consensus, not a measurement; luna's own caveat is that a floating
line on real silicon may read high.

Reproduce (2026-09-22): `snes_search("What does an unplugged (empty)
controller port read: auto-joypad $4218/$4219 value with no controller, and
manual $4016/$4017 serial clocking past bit 16 — data line idle high or
low?", exclude_sources=[…], k=6)` → sfc-dev-wiki JOY1L/JOYSER1 bit maps
(`d867f1d7f4d8c40a`, `36528163bfc15466`), anomie-timing's auto-read timing
(`60e7a00b7d843371`), wikibooks' "0000 = standard controller"
(`5dee6ebd90c1db9d`) — the bit layouts and the timing, never the value of
an empty port.

**Ask:** either point us to a source that states it (we would rather cite
than measure), or carry it as what it is — *"modelled identically by three
emulators, unmeasured"* — so the next person who asks gets that answer
instead of nothing. It is on our real-console checklist
(`docs/HARDWARE_VERIFICATION.md`); we will send the trace when we have one.

### S3 — DSP-1 `Distance`: one fact measured on the real firmware, and a bug sneslab names but does not describe

**Kind:** a measured fact + a passage to complete. **Cost:** editorial.

First, a correction of our own record. Our 2026-09-20 note to luna said
that "no hardware reference we could find states" what `Range` returns.
That was wrong, and the corpus proves it: the official manual, Book II
§5.2.2 (`nintendo-devmanual-book2`, chunk `5aaea1619232b3b3`), gives
`Range` = code 18H, output `D[T/H2]`, and "subtracts the square of the
specified range from the square of the vector size". Our header had
described a raw difference; the manual said squared all along. Withdrawn
as a corpus gap — it was a reading gap, and a query would have caught it.

What remains, on `Distance` (code 28H, manual §5.2.3, chunk
`a0ebc4dee389b377`, output `R[I/T]`, no statement about rounding):

- **Measured on the DSP-1B firmware in luna** (`devtools/libtests_dsp1`,
  17 vectors): `Distance` reads **one low on exact lengths** — (3, 4, 12)
  returns 12, not 13; (300, 400, 0) returns 499; (0, 0, 10000) returns 9999.
- **sneslab-wiki** (`DSP1/Distance`, chunk `d789432d05d502eb`) says "It is
  bugged in DSP1/DSP1A and fixed in DSP1B" and cites bsnes
  `dsp1emu.cpp#L395` — but never says what the bug is. Our one-low result
  is on the *fixed* chip, so either the 1B behaviour is the intended
  rounding (truncation of √), or the "fix" is something else.

**Ask:** (a) capture the bsnes `dsp1emu.cpp` comment sneslab points to, or
whatever states what the DSP1/1A bug was, so the passage stops being a
dangling claim; (b) carry the 1B measurement as *"measured on DSP-1B
firmware under luna, 2026-09-20; consistent with truncating √"* until a
reference states the rounding.

Reproduce our queries:
`snes_search("DSP-1 command $28 Distance exact result rounding … command $1A Range …")`
→ sneslab `d789432d05d502eb`, `d6a1ce185520a302`; the manual surfaced only
on the second, sneslab-vocabulary query (`snes_search("sneslab DSP1 Range
opcode 1A input parameters output D …")` → `5aaea1619232b3b3`). Note the
first query used the wrong code ($1A for $18) and still found sneslab, not
the manual — the manual's "Code: 18H" is the token that ranks it.

### S4 — our own two sources are three weeks stale

**Kind:** re-capture. **Cost:** small.

`opensnes-docs` and `opensnes-notes-tech` were captured 2026-09-03. Since
then: 20 headers rewritten, 27 tutorials (three new: framework, object,
text), the hardware
verification protocol, `KNOWN_LIMITATIONS.md` with the v0.44.0 entries,
and the DSP-1 facts above. They are the negative-control source of our
golden queries (`N`: cc65816 push order must be answered by us, never by
`qbe-docs`), so their staleness weakens that control too.

**Ask:** re-capture both from the `v0.44.0` tag, and ideally at every
release tag from now on (the tag is the natural capture point).

### S5 — Super FX is a second-class domain in the corpus, and we are about to live there

**Kind:** coverage + authority. **Cost:** ingestion (some sources are already
listed) and labelling.

We are sizing a multi-week chantier to make real GSU games possible
(`.claude/notes/reviews/2026-09-24_superfx_game_gaps.md`). Preparing it,
we ran six GSU queries on 2026-09-24; **four came back "aucune source
arbitre"** — the answers were right but came from complement/solid sources
(sneslab, wikibooks, oldmachines, stuntrace-recomp) and from the Nintendo
manual's OCR, whose GSU tables are garbled (`<!-- formula-not-decoded -->`,
register bit tables flattened). For every other hardware topic this cycle
the arbiters answered first.

What we need the corpus to carry, in the order we will need it:

1. **An arbiter for the GSU itself.** fullsnes has a Super FX section
   (registers, opcodes, timing) that never surfaced on our queries — is it
   captured under a heading the ranking does not reach, or not chunked
   as such? If fullsnes covers it, we would like it to answer; if not,
   the manual's Book II chapters 4-6 (registers, execution, interrupts)
   are the reference, and a clean capture of them (the tables, not the
   OCR) would be the single most useful addition. `gsu-development-kit`
   is listed as complement and reportedly transcribes the ISA — worth
   checking whether it can stand in.
2. **Production GSU code as a domain arbiter.** `doom-fx-source` already is
   one; the gigaleak source of **Star Fox / Star Fox 2** (Argonaut's own
   CPU/GSU split, the `$0100-$010F` WRAM interrupt stubs, the frame
   pipeline, ARGSFX macros) is the best context that exists for the
   exact problem we have — is it inside `retroreversing-gigaleak`, and if
   so, can the GSU-relevant tree be indexed so a query on "how Star Fox
   keeps its NMI alive during a GSU job" reaches it?
3. **The tooling side:** `casfx` (captured, solid) and `libsfx`'s GSU macro
   pack are what we will model our macro library on; the SuperFX3 project
   is captured. Missing: the ARGSFX assembler documentation if any copy
   exists, and byuu/Near's GSU notes (cache and pipeline behaviour, the
   MC1 store→STOP quirk our expert note mentions) if they were ever
   written down.
4. **One hardware fact to settle with an arbiter:** does the FXPak Pro run
   Super FX? The corpus answers only by omission (`sfc-dev-wiki`'s chip
   list `4ec0785bcc6b469b` lacks it). sd2snes's own feature list or
   changelog would make it a citation instead of an inference.
5. **Two claims to verify** that we currently cite from complement
   sources: the dummy-byte table a CPU read of ROM returns under GSU
   ownership (`sneslab` `4a1e3a154e8eb7c7`) and the `$0108`/`$010C`
   vector convention (confirmed only by Stunt Race FX's header). If
   fullsnes or the manual states them, we want the arbiter's wording.

Golden queries for this domain, to add to `cartouche_corpus.md` once the
sources land: the six of the review's §4 (reproduce list there), with the
expected top source next to each.

## 3. Priority, from our side

| # | Request | Why it matters to us |
|---|---|---|
| S5 | Super FX coverage and arbiters | a multi-week chantier starts on it; four of six queries had no arbiter |
| S1 | re-capture `luna-docs` at every luna tag | the corpus is how a session learns luna's flags without re-reading `--help` |
| S4 | re-capture our two sources at `v0.44.0` | our negative control, and the DSP-1 facts ride along |
| S2 | the unplugged-port model, as a stated hypothesis | blocks one public function's fix from being *cited* rather than *asserted* |
| S3 | the `Distance` bug named, the 1B rounding carried | so the next SDK does not rediscover it — and so we stop citing "no reference" when there is one |

## 4. Small observations (no ask)

- On the `$4017` query, fullsnes's passage came with the documented-error
  banner about CGWSEL bits 4-5 vs 6-7. Right banner, wrong topic: the error
  is real but lives elsewhere in the same "Unpredictable Things" chunk. If
  banners can be keyed to the sub-section rather than the chunk, this one
  would stop looking like a warning about joypad bits.
- `snes_verify` remains the right tool for a *sentence* and `snes_search`
  for a *question*; the cartouche_corpus.md note records the one
  `confirmed` that cited a generic passage. No change asked, just the
  reminder that we compare wording to citation every time.
- Golden queries 2 (QBE `call` with env / variadic marker) and 7 (TMX flip
  flags) were ✗ on 2026-09-12; they will be re-run at the 1.26.0 pin bump
  and reported here if still ✗.
