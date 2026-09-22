# The two shoulders: luna and snes-rag (Auto-loaded)

CRITICAL, owner decision (2026-09-22): OpenSNES does not work alone. It
stands on two partners, and the three of us work **jointly** — each one
helps the other two, and each one owes the other two its feedback.

| Partner | What it is | What it gives us | What we give it |
|---|---|---|---|
| **luna** | the cycle-accurate emulator, test backend and debugger (`k0b3n4irb/luna`, pinned in `tools/luna-test/luna.version`) | the one source of truth for running, inspecting and judging ROMs (`.claude/rules/luna_tooling.md`) | capability requests specified by a working prototype, bug reports with a ROM and a command line, and the answer to every note it sends us |
| **snes-rag** (the Cartouche corpus, MCP `cartouche`) | the arbitrated SNES reference corpus: hardware, chips, SPC700, formats, toolchain docs, luna's own docs | the arbiter for every hardware claim and every "is this the hardware or the toolchain?" question (`.claude/rules/hardware_claims.md`) | every query that came back empty, wrong or unsettled; every source it is missing; every fact we established that no source states |

This rule is the contract between the three. The two rules it cites say
*how* to use each partner; this one says that using them is not optional,
and that **silence is a failure**: a gap noticed and not reported is the
same gap next month.

## The duty of feedback

Do not hesitate — the owner has said so explicitly. If something is
missing, wrong, or merely inconvenient in a partner, write it down for them
the same day, with what you tried and what you needed. Concretely:

- **To luna** (active partner, answers and ships): when a measurement,
  flag, device, oracle or diagnostic is missing; when its output
  contradicts a reference; when a note it sent us needs an answer.
  Lifecycle in `luna_tooling.md`: prototype → owner validates → luna issue
  (`gh issue create --repo k0b3n4irb/luna`) → luna ships → we delete the
  prototype. A *report* (several requests, priorities, what luna made
  possible since the last one) goes through the owner as a file.
- **To snes-rag** (passive partner: it waits for our feedback and luna's,
  it does not come to us): when `snes_search` / `snes_verify` returns
  nothing, the wrong source, an `unsettled` on a point some reference does
  settle, a passage that is truncated where it matters, a documented-error
  flag that is wrong; when a source we need is not captured; when a fact we
  had to *measure* (on luna, on a console, on a real firmware) is stated by
  no source — that is a fact the corpus should carry, with its provenance;
  when luna ships a version whose docs the corpus has not re-captured.
- **luna ↔ snes-rag** — we are the bridge. When luna answers a hardware
  question (an unplugged port, the `$4017` tied bits, the DSP-1 firmware's
  real `Distance`), that answer and its arbitration status go into the
  snes-rag report so the corpus can carry it. When the corpus contradicts
  luna, luna gets the citation.

## Where the exchanges live

`.claude/notes/partners/<partner>/YYYY-MM-DD_<direction>_<topic>.md`, with
`direction` = `to` (ours) or `from` (theirs). Versioned in the repo, not in
`/tmp` or on one machine: a report that lives only in `/tmp` is lost with
the session, and the *answers* to it are project knowledge. The owner
forwards our `to_` files and drops the partner's replies in as `from_`
files (or in `/tmp`, from where we copy them in).

Every `to_` report has the same spine: header (who, which pin / index
fingerprint, status "proposal — owner validates before filing"), what the
partner made possible since the last report (first — it is the context for
the asks), the requests **simplest first** with kind, cost and the exact
I/O contract we would use, a priority table, small observations without an
ask. Precedents: `partners/luna/2026-09-20_to_luna_report.md`,
`partners/snes-rag/`.

## What each session does

1. **Before blaming the toolchain**, query snes-rag (`hardware_claims.md`
   step 4). **Before writing an internal script**, ask what luna already
   exposes (`luna_tooling.md`).
2. **When a partner falls short**, append the item to the *open* report for
   that partner (`partners/<partner>/OPEN_<partner>.md`, created on first
   need) — one line is enough: date, what was asked, what came back, what
   was needed. The owner decides when it is sent; a sent report is renamed
   to its date.
3. **When a partner sends something**, answer it in full and in writing
   (`to_..._reply.md`), correction by correction: what we take, what we
   withdraw, what we still need. Their corrections of *us* go into our
   notes the same day (precedent: R1 and R4 of the 2026-09-20 exchange
   changed two of our conclusions).
4. **At every luna pin bump**: re-run the corpus golden queries
   (`.claude/notes/tech/cartouche_corpus.md`) and tell snes-rag if
   `luna-docs` lags the tag.

## When this rule does NOT apply

- Non-SNES, non-toolchain questions (git, Python, CI syntax): no partner
  owns those.
- Re-reporting an item already on the open report — add evidence to the
  existing line instead.

## Cross-references

- `.claude/rules/luna_tooling.md` — Luna-First and the prototype lifecycle.
- `.claude/rules/hardware_claims.md` — arbitration of hardware claims.
- `.claude/notes/tech/cartouche_corpus.md` — what the corpus holds, its
  fingerprint, the golden queries.
- `.claude/notes/partners/` — the exchanges themselves.
