# OpenSNES Project Notes

Versioned, shared project knowledge. **For contributors and future
maintainers**: this directory captures the lessons, patterns, technical
references, and active work that aren't normative rules but matter for
understanding *why* the project is the way it is.

This is the durable counterpart to Claude Code's auto-memory system. The
auto-memory at `~/.claude/projects/.../memory/` is per-user and not
shared; project-relevant knowledge lives here in the repo so it survives
clones, moves, and contributor turnover. See
[`.claude/rules/memory_routing.md`](../rules/memory_routing.md) for the
routing convention.

## What lives where

| Directory | Purpose |
|-----------|---------|
| [`conventions/`](conventions/) | Lessons learned about *how we work*. Commit etiquette, audit hygiene, debugging discipline, communication style. The "we got burned, now we know" content. |
| [`chantiers/`](chantiers/) | Active multi-day chantier handoffs. Each file is a complete state-of-play for a chantier in progress, so a new session can pick up cleanly. |
| [`patterns/`](patterns/) | Reusable workflow patterns: porting from PVSnesLib, asset pipeline gotchas, SNES dev best practices. |
| [`tech/`](tech/) | Technical references and analyses: HDMA quirks, compiler optimization history, SuperFX expert feedback. |
| [`status/`](status/) | Current state of moving parts: emulator status, pending validations, partial chantiers. Updated when state changes. |
| [`archive/`](archive/) | Historical context: bugs that were FIXED, decisions made, kept for context if a related symptom resurfaces. |
| [`reviews/`](reviews/) | Structured project reviews with ISO date prefixes (`YYYY-MM-DD_topic.md`). |
| [`projects/`](projects/) | Sub-project notes (e.g. `rpg.md` for the RPG project under `projects/rpg/`). |

## Conventions

- **One concept per file.** Don't bundle unrelated topics; granularity makes
  updates and cross-references cleaner.
- **Frontmatter where useful.** A `name`/`description`/`type` block at the
  top helps future readers (and Claude sessions) decide relevance fast.
- **State, then rationale, then how-to-apply.** Every file should answer
  "what is this", "why does it matter", "what should I do with it".
- **Date everything that is time-bound.** Fixed bugs, completed chantiers,
  validated decisions — note the date so a future reader knows whether
  the entry is still load-bearing.
- **Drop, don't bury.** If an entry becomes obsolete (the bug is fixed,
  the chantier ships), either move it to `archive/` with a "FIXED on
  YYYY-MM-DD" marker, or delete it. Don't accumulate stale content.

## What does NOT go here

- **Normative rules** (must-do conventions, auto-loaded by Claude). Those
  live in [`../rules/`](../rules/).
- **User-specific cross-project preferences** (a contributor's personal
  workflow habits). Those stay in `~/.claude/projects/.../memory/`.
- **Generated artifacts** (build logs, ROM dumps, captured asm). Those
  are transient and don't need versioning.
- **Anything documented elsewhere already.** Cross-link to
  `KNOWN_LIMITATIONS.md`, `ROADMAP.md`, `compiler/PINS.md`, etc., rather
  than duplicating.

## Migration history

The bulk of `notes/` was migrated from `~/.claude/projects/.../memory/`
on 2026-05-10. Pre-migration, this content was per-user and lost on
project moves. See the migration commit on `develop` for the per-file
mapping if you need to trace an entry back to its previous location.
