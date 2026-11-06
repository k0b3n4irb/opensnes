# Memory routing (always loaded)

Project knowledge goes to `.claude/notes/` in the repository (versioned, survives clones), not to the per-user `~/.claude/projects/.../memory/` (lost on a machine move). The user memory keeps only personal cross-project preferences. When in doubt, the repo.

| Type | Folder |
|---|---|
| lessons about how the project works | `notes/conventions/` |
| active multi-day chantier | `notes/chantiers/` |
| reusable workflow | `notes/patterns/` |
| technical reference or analysis | `notes/tech/` |
| current state of a moving target | `notes/status/` |
| fixed bugs kept for context | `notes/archive/` |
| structured reviews, dated | `notes/reviews/` |
| reports to and from luna and rag-partner | `notes/partners/<partner>/` (`partners.md`) |
| what concerns a game alone | `notes/private/` (ignored by git, never committed) |

Rules that must trigger on their own are `.claude/rules/`; notes record context, rules enforce behaviour. Generated artifacts (build logs, ROM dumps) are transient and go nowhere. Layout: `.claude/notes/README.md`.

Notes follow the showcase like every other file of this repository, resume notes included: no private project name in `.claude/notes/` (a partner port, the conductor, the porting kit, the Amiga oracle, the rag-partner are the neutral names). The pre-push hook refuses the private words; a name that slipped in is cleaned from the current files by a normal commit.
