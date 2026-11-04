# luna, snes-rag and the game: working jointly (always loaded)

Roles, authorisations, pushes, "one lot = one conversation" and the scientific method are in the tree's `CLAUDE.md`, not here. This file is what is specific to OpenSNES.

## The partners
| Partner | What it is | We owe it |
|---|---|---|
| **luna** | cycle-accurate emulator, test backend and debugger (`k0b3n4irb/luna`, pinned in `testing/luna.version`) | capability requests with a working prototype, bug reports with a ROM and a command line, an answer to every note it sends. Lifecycle in `luna_tooling.md`. |
| **snes-rag** (corpus, MCP `cartouche`) | arbitrated SNES reference: hardware, chips, SPC700, formats, luna's docs | every query that came back empty, wrong or unsettled; every source it lacks; every fact we measured that no source states (with provenance). It has no standing session. |
| **the game** | the first project built on the SDK, the proof that the whole assembles | what it shows us stays in `.claude/notes/private/` (ignored by git); in this repository it is "the game", nothing more. |

**Silence is a failure**: a gap in a partner noticed and not reported the same day is the same gap next month. We are the bridge: what luna answers about the hardware goes into the snes-rag report; when the corpus contradicts luna, luna gets the citation.

## How to talk
- `ListAgents`, then `SendMessage` to `luna` and to the game's session (recognised by working directory). No GitHub issue between the three. First line: what it is about; then weight (`léger` / `moyen` / `lourd`; **anything touching code generation is `lourd`**), whether it blocks, the fact with its piece, what is asked, when it closes. A message is read at a natural break; only what blocks goes ahead of the work in hand.
- **A claim comes with its piece** (measurement, command, ROM, test) and is provoked before it is stated to a neighbour. Two round trips without agreement: the persistent disagreement goes to the conductor.
- **What is decided is written down** (notes, `CHANGELOG`); a long piece is a dated report under `.claude/notes/partners/<partner>/YYYY-MM-DD_{to,from}_<topic>.md`, announced by message, copied by the receiver. An unfinished report is `OPEN_<partner>.md`, one line per item (date, asked, came back, needed). Every claim in a report was re-checked the day it is sent. snes-rag's folder is reached by files only: ours are delivered to the `opensnes/` folder of its partner notes (location in the user memory).

## The SDK learns from the game; it does not bend to it
A request lands only if it passes four checks, written in the reply:
1. **The need is general**: a second game would hit it (a game reading the lib's internals is the usual sign the SDK lacks something). One case is not enough: it waits for a second.
2. **The API speaks the SDK's terms** (bytes, entries, frames, banks), from a stand-alone reproducer, never the game's sources.
3. **Policy stays with the game**: the SDK gives the measure and the mechanism (precedent: `vramQueueCost()` yes; a budget imposed by `vramQueuePush()` no, `01fe79bf`; `vramQueuePushWithin()` yes, the budget is an argument).
4. **The cost to every other user is small and stated**, measured in the commit.
A justified no closes a request; the asker finds another way or brings a new fact.

## The game lives on this tree
It builds against this working tree on `develop`, never a release. So: a compiler chantier, a change to a public header or to `make/common.mk` is made in a **separate worktree** (an uncommitted edit here is seen by the game's next build); what lands on `develop` is announced to it; a full rebuild (`make clean && make`, `bin/` incomplete for minutes) is announced before and after; during a Class A validation, give it the compiler binary in a separate directory. Nothing of the game enters a commit, test, note, document or commit message: name, rights holder, paths, source file or function names, measurements tied to its sources. What must enter is a stand-alone example written by us. A worktree opened by a sub-agent starts from the last release, not from `develop`: say `git checkout -B work develop` first.

## Periodic duties
- Before blaming the toolchain: query snes-rag (`hardware_claims.md`). Before writing an internal script: ask what luna exposes (`luna_tooling.md`).
- At every luna pin bump: re-run the corpus golden queries (`.claude/notes/tech/cartouche_corpus.md`) and tell snes-rag if `luna-docs` lags the tag.
- Weekly: the five SNESMOD issues (`.claude/notes/partners/snesmod-upstream/README.md`, with the command and the next due date). Monthly: what QBE upstream committed, to take what serves us, nothing goes back (`.claude/notes/partners/qbe-upstream/README.md`).
