# Hardware Claims Under Arbitration (Auto-loaded)

CRITICAL: every **new hardware claim** written into `docs/` or
`KNOWN_LIMITATIONS.md` — a register's behaviour, a timing figure, a
PPU/APU/chip property — must be checked against the Cartouche corpus
(MCP `cartouche`, tool `snes_search`) before it lands. The project's
two costliest historical detours came from exactly one pattern: *a
wrong or misread external source, with no arbiter consulted at the
moment of doubt*.

- **SA-1 SIWP polarity** — the Super Famicom Dev Wiki page states the
  inverse of fullsnes, the official Nintendo manual and nocash's
  hardware debugging. Trusting it broke SA-1, and the revert left a
  "disputed" status plus defensive warnings in four files for six
  months. One arbitrated query resolves it (see the 🟢 entry in
  `KNOWN_LIMITATIONS.md`).
- **HDMA "Direct Mode BROKEN on this toolchain"** — a misreading of
  the HDMA table format (per-line data laid out under a non-repeat
  count) produced "straight lines", got blamed on the assembler, and
  hardened into a false "registers forget → repeat required" doctrine
  in two tutorials. The corpus states the format plainly (anomie-regs).
  See `.claude/notes/tech/hdma_notes.md` and the retrospective in
  `.claude/notes/chantiers/hardware_docs_audit.md`.

## The workflow

1. **When writing a hardware claim** — query first:
   ```
   snes_search(question, exclude_sources=["opensnes-docs", "opensnes-notes-tech"])
   ```
   The exclusion is mandatory for verification queries: our own docs
   are indexed in the corpus, and letting them answer turns the check
   into a confirmation loop (they are the thing being verified).
2. **When sources conflict** — `contrast=true` lays every source's
   position side by side, ordered by authority, with anchored
   citations. This is the tool for disputed polarities and timings.
3. **When no arbiter settles it** — write the claim as a *hypothesis*,
   not a fact ("observed; mechanism unconfirmed by references").
   Precedent: the fixMul/auto-joypad mechanism note in
   `KNOWN_LIMITATIONS.md`. An honest "unconfirmed" beats a confident
   guess that calcifies.
4. **When debugging an unexplained symptom** that could be hardware
   behaviour: query the corpus BEFORE blaming the toolchain. The
   toolchain is guilty sometimes — but "hardware behaves differently
   than I assumed" is the cheaper hypothesis to eliminate, and the
   corpus eliminates it in one query.
5. **Traceability** — a non-trivial claim cites its arbiter source
   (anchored link) in the doc itself or in the commit message.
   `snes_get(chunk_id)` retrieves the full passage when the search
   excerpt is truncated.

## When this rule does NOT apply

- Non-hardware prose: SDK API decisions, build-system behaviour,
  workflow docs, examples' gameplay text.
- Claims already carrying an arbiter citation (re-verifying on every
  edit is noise; re-verify when the *claim itself* changes).
- Sessions where the cartouche MCP is not connected: degrade
  gracefully — mark the claim "to verify against the SNES corpus" in
  the commit body so a later session picks it up. Do not silently
  skip the check.
- CHANGELOG entries and archived notes (frozen history by design).

## Cross-references

- `.claude/rules/regression_method.md` — the debugging twin of this
  rule (bisect, never guess); this rule adds "arbitrate, never assume"
  on the documentation side.
- `.claude/rules/doc_consistency.md` — mechanical doc/code drift; this
  rule covers *factual* drift the sentinel cannot see.
- `.claude/notes/chantiers/hardware_docs_audit.md` — the 2026-09 audit
  that motivated this rule (10 findings, all documentation-side; the
  lib was already correct everywhere).
