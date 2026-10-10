# Direct exchanges with luna and the game (Auto-loaded)

CRITICAL, owner decision (2026-10-10). OpenSNES, luna and the game
(speedball2) talk to each other **directly, session to session**, and
challenge one another. The shared charter is
`~/workspace/snes-tutor/protocole/ECHANGES.md` (French): read it at the
start of a session, it is the reference. This rule says what it changes
here; where it and an older rule disagree on *how a partner is reached*,
this one wins.

## Three roles

OpenSNES is the body, luna is the sight, and the game is the confirmation
that the whole assembles: the proof that a real game is built with the two.
A gap of ours the game works around in silence is a failed proof.

## Two axes, both at once

1. **Our own work**: our bugs, our features, our roadmap. A neighbour asks,
   it does not command. A message is read at a natural break, never in the
   middle of a validation; only what **blocks** a neighbour goes ahead of
   the work in hand.
2. **Collaboration**, so that we do not isolate ourselves — more than
   answering requests:
   - show what just landed on `develop` to whoever it touches;
   - ask before finishing: an API is shown to the game that will call it
     (first case: the shape of `opensnes-tileset`'s column-major map, sent
     before a line was written);
   - say what rubs when we use a neighbour's work, with nothing to ask.

## How

- **`ListAgents`, then `SendMessage`**, to the sessions `luna` and
  `speedball2` (recognised by their working directory when they do not
  carry the name yet). **No GitHub issue between the three any more**: a
  capability request to luna is a direct message with its prototype, not
  `gh issue create`. The issues already open continue directly; closing one
  on GitHub is a public act and waits for the owner.
- Message format, weight (`léger` / `moyen` / `lourd`) and the right to
  contest are in the charter. **Here, anything that touches code generation
  is `lourd`**, even in ten lines. Two round trips without agreement: the
  thread goes to the session `snes-tutor`, which arbitrates.
- **A claim comes with its piece** (a measurement, a command to replay, a
  ROM, a test); and before stating a limit at a neighbour's, provoke it.
- The game may show us its real case by path, on this machine. **Nothing of
  the game enters a commit, a test or a document here** (rights: Rebellion);
  what has to enter is a stand-alone example.
- **What is decided is written here** (notes, `CHANGELOG`, the project
  note): a message disappears with its session. A long piece is still a
  dated report (`.claude/notes/partners/`, `~/workspace/partner-reports/`);
  the message points to it. snes-rag has no standing session: it is still
  reached by files (`partners.md`).
- **Trace**: one line in `~/workspace/snes-tutor/registre/BOITE.md` when a
  thread opens and when it closes (format in the charter).

## The game lives on this tree

The game builds against this working tree, on `develop`, as it is built —
never a release.

- A compiler chantier goes in a **separate worktree** (`release.md`, the
  one-session-per-tree point), not in the tree the game consumes.
- What lands on `develop` for a neighbour is **announced to it**: the
  commit, what changes for it, what to try again.
- A full rebuild of this tree (`make clean && make`, minutes during which
  `bin/` is incomplete) is **announced to the game before and after**.
- A lot is never smaller than its validation. While a Class A validation
  runs, give the game the compiler binary in a separate directory, said as
  such, so it can measure meanwhile.
- The luna pin (`testing/luna.version`) and the releases stay what they are
  for the public and the CI; they no longer pace the work between the three.
