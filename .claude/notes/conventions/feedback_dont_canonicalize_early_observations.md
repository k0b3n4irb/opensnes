---
name: Don't canonicalize early empirical observations without re-validation
description: An observation from early development can become a "fact" in canonical docs (README/CLAUDE.md/ROADMAP) and survive long after it stops being true. Always question entries that say "X has a confirmed bug" without an upstream issue link or a current reproducer.
type: feedback
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
When you see a doc claim that some external tool or emulator "has a
confirmed bug", treat it as **suspect until re-validated**, not as
ground truth — especially if:

- The claim was added during early/experimental work on a feature
  (e.g., chantier kickoff, first integration of a new chip).
- It comes with no link to an upstream issue tracker, no minimal
  reproducer in the repo, no bisection.
- The wording is "confirmed via comparison" / "we observed" rather
  than a specific test case anyone can re-run.

**Why this matters here:** OpenSNES carried a "Mesen2 has a confirmed
SuperFX backward-branch bug" claim in `README.md`, `CLAUDE.md`,
`KNOWN_LIMITATIONS.md`, the ROADMAP P3.4 entry, and two example READMEs
for over a month. The original observation (March 22, 2026, commit
d330627, `.claude/SUPERFX.md`) compared a bsnes run against a Mesen2
run during early SuperFX bring-up and concluded Mesen2 was buggy. It
turned out the snes9x-side of the same comparison had been giving a
"false positive" through partial GSU emulation; Mesen2 was actually
the correct, stricter reference all along. By the time we
re-validated (April 29, 2026), the claim had been in canonical docs
for five weeks and was being cited as justification for a `P3.4`
roadmap item ("integrate bsnes-headless because Mesen2 can't be
trusted"). That justification was inverted: we want **Mesen2**-headless
for CI, not bsnes-headless.

**How to apply:**

1. Before you cite a "X is broken" claim from project docs in a chain
   of reasoning (especially when proposing follow-up work like new
   tooling), open the file referenced as the source and check whether
   it actually shows a reproducer, a test case, or just an early
   observation.
2. If you propagate a doc claim verbatim into your own analysis,
   prefix it with the acknowledgement that you have not re-checked it
   ("per `KNOWN_LIMITATIONS.md`, ...; have not re-validated today")
   so the user can challenge you cheaply.
3. When the user pushes back on a claim with "qui a dit ça ?",
   actually trace it to the originating commit/file, don't restate
   it from memory. The question is the user's invitation to verify.
4. When **adding** a new doc entry of the form "X has a known bug",
   either link an upstream issue / commit / reproducer, or word it
   tentatively ("we observed X on date D in configuration C; not
   re-validated since"). Don't write canonical-sounding prose that
   then propagates across the codebase.
