# OpenSNES → snes-rag : open report (accumulating since 2026-09-22)

| | |
|---|---|
| **From** | OpenSNES SDK (`k0b3n4irb/opensnes`, `develop`, post-v0.44.0) |
| **Corpus seen** | 201 of 223 sources, 30848 chunks, fingerprint `e2a738a553ec` (built 2026-09-12) — unchanged since our last validation report |
| **Status** | proposal — the owner validates before sending (`.claude/rules/partners.md`); items are appended as they come up, the file is renamed to its date when sent |

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

**Ask:** either point us to a source that states it (we would rather cite
than measure), or carry it as what it is — *"modelled identically by three
emulators, unmeasured"* — so the next person who asks gets that answer
instead of nothing. It is on our real-console checklist
(`docs/HARDWARE_VERIFICATION.md`); we will send the trace when we have one.

### S3 — two DSP-1 facts measured on the real firmware that no reference states

**Kind:** measured facts, provenance = luna running the DSP-1B firmware.
**Cost:** editorial.

Running the real `dsp1b.rom` in luna contradicted our own header twice, and
we could find no reference (mame-upd7725, the NEC bitsavers manuals,
jsgroth-dsp1, nesdev-thread-dsp1-homebrew) that states either:

- `Distance` ($28): reads **one low on exact lengths** — (3, 4, 12) returns
  12, not 13; (300, 400, 0) returns 499.
- `Range` ($1A): returns **(x² + y² + z² − r²) >> 15**, the squared
  difference shifted, not the raw difference the docs describe.

Both are pinned by `devtools/libtests_dsp1` (17 vectors against the real
firmware) and documented in `lib/include/snes/dsp1.h`.

**Ask:** which source should carry these — a re-capture of
`opensnes-notes-tech` / `opensnes-docs` (see S4), or a dedicated "measured
facts" entry with the provenance above? We will keep them marked *measured,
not referenced* until a reference appears.

### S4 — our own two sources are three weeks stale

**Kind:** re-capture. **Cost:** small.

`opensnes-docs` and `opensnes-notes-tech` were captured 2026-09-03. Since
then: 20 headers rewritten, 24 tutorials (three new), the hardware
verification protocol, `KNOWN_LIMITATIONS.md` with the v0.44.0 entries,
and the DSP-1 facts above. They are the negative-control source of our
golden queries (`N`: cc65816 push order must be answered by us, never by
`qbe-docs`), so their staleness weakens that control too.

**Ask:** re-capture both from the `v0.44.0` tag, and ideally at every
release tag from now on (the tag is the natural capture point).

## 3. Priority, from our side

| # | Request | Why it matters to us |
|---|---|---|
| S1 | re-capture `luna-docs` at every luna tag | the corpus is how a session learns luna's flags without re-reading `--help` |
| S4 | re-capture our two sources at `v0.44.0` | our negative control, and the DSP-1 facts ride along |
| S2 | the unplugged-port model, as a stated hypothesis | blocks one public function's fix from being *cited* rather than *asserted* |
| S3 | a home for measured facts | so the next SDK does not rediscover them |

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
