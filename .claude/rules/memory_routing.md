# Memory Routing (Auto-loaded)

CRITICAL for Claude sessions: project knowledge belongs in
`.claude/notes/` in the repo, NOT in `~/.claude/projects/.../memory/`.
The latter is per-user and lost on project moves; the former is
versioned, shared, and survives clones. Migration completed 2026-05-10.

## The two buckets

| Bucket | Path | Versioned | Visible to contributors |
|---|---|---|---|
| **Project notes** | `.claude/notes/` (repo) | ✅ git | ✅ on clone |
| **User auto-memory** | `~/.claude/projects/-home-kobenairb-workspace-opensnes/memory/` | ❌ | ❌ |

## What goes where

| Type | Bucket | Examples |
|---|---|---|
| Lessons learned about *how the project works* | `.claude/notes/conventions/` | "no Co-Authored-By in commits", "validate before commit" |
| Active multi-day chantier handoff | `.claude/notes/chantiers/` | A7 Phase 0 handoff |
| Reusable workflow patterns | `.claude/notes/patterns/` | gfx4snes 2bpp, PVSnesLib porting |
| Technical references / analyses | `.claude/notes/tech/` | HDMA notes, compiler optimizations, SuperFX expert feedback |
| Current state of a moving target | `.claude/notes/status/` | luna harness status, pending validations |
| FIXED bugs preserved for context | `.claude/notes/archive/` | mktype UB, JSL codegen bug |
| Structured project reviews | `.claude/notes/reviews/` | dated `YYYY-MM-DD_topic.md` |
| Sub-project notes | `.claude/notes/projects/` | RPG project asset references |
| **User-specific cross-project preferences** | `~/.claude/projects/.../memory/` | personal grep style, editor habits |

## Routing rule for new entries

When you (Claude session) learn something worth saving:

1. **Is it about THIS project specifically (any concept, file, or
   contributor will reference it)?** → write to
   `.claude/notes/<category>/<descriptive_name>.md` in the repo.
2. **Is it a personal cross-project user preference (the user's grep
   style, terminal habits, etc.)?** → write to
   `~/.claude/projects/.../memory/` and register in the home `MEMORY.md`.

When in doubt, default to `.claude/notes/` — being slightly too generous
on project-bucket means a contributor sees something they don't need;
being too generous on user-bucket means losing project knowledge to a
machine reset.

## Cross-references

- [`.claude/notes/README.md`](../notes/README.md) — directory layout and
  conventions
- The auto-memory system instructions (system prompt) still mention
  `~/.claude/projects/.../memory/` as the memory path — that hardcoded
  path doesn't override this rule. Use it for the user-pref bucket
  ONLY.
- `~/.claude/projects/.../memory/MEMORY.md` — slim index pointing back
  to `.claude/notes/` after the 2026-05-10 migration.

## When this rule does NOT apply

- Normative rules that should always trigger / be auto-loaded — those
  belong in `.claude/rules/`, not `.claude/notes/`. The split is:
  *rules enforce behavior, notes record context.*
- Generated artifacts (build logs, ROM dumps). Those are transient.

## Why this rule exists

Pre-2026-05-10, project memory lived under `~/.claude/projects/.../memory/`
because that's what Claude Code's auto-memory system writes to by
default. After the SDK reached real maturity (v0.17.0, multi-platform CI,
contributor-ready MIT licence), keeping project knowledge in a
per-machine directory became the wrong default — see the migration
commit on develop (`chore(claude): migrate project memory ...`) for
the per-file move, or `.claude/notes/README.md` for the structure.
