---
name: Never compromise — capture the root cause, no economy
description: When a fix doesn't work and looks "hard to solve", do NOT propose to revert / skip / accept a workaround. The diagnosis is incomplete; dig deeper until the actual origin is identified. Economizing on diagnostic effort is the trap.
type: feedback
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
When a CI failure (or any bug) resists the obvious fix, the user's
explicit rule:

> **« On doit capter l'origine du problème ! Tu dois ajouter dans
> ta rosastone : ne jamais transiger, pas d'économie. Si ce n'est
> pas solvable, le problème est ailleurs. »**

**Why:** repeated pattern in this project — I tend to propose
workarounds (revert, skip, "live with it") when a fix attempt
fails on the first try. That's economizing on diagnostic effort.
The user wants the actual root cause identified, not papered
over. "If it's not solvable, the problem is elsewhere" — meaning
my mental model is wrong, not that the bug is intractable. Specific
trigger: P3.4 Mesen2 CI phase still SIGABRT'ing after xvfb fix; I
proposed reverting P3.4 from CI. The user refused: dig deeper.

**How to apply:**

1. **When a fix attempt fails, do NOT propose to revert / skip /
   accept the limitation.** That's the wrong reflex on this
   project. The user reads it as "you gave up too early".

2. **Add diagnostic capability to the failing layer.** If the test
   runner doesn't surface the underlying error (e.g. stderr from a
   subprocess, full log from CI, kernel coredump), instrument it
   first. Push instrumentation, observe, then fix from real data.

3. **Cross-check past assumptions when stuck.** Memory entries 60+
   days old can mislead — see also
   `feedback_dont_canonicalize_early_observations.md`. Re-validate
   "X is broken" claims against current state before acting on them.

4. **Layer-hop honestly.** "Mesen2 NativeAOT crashes" can be:
   - the wrong settings.json template
   - a missing library on the runner image (libfontconfig,
     libsdl2-dev, etc.)
   - a Mesen2 version mismatch with the .NET runtime present
   - a different binary than what works locally
   Don't stop at the first plausible-sounding cause; check each.

5. **Only after the root cause is captured and verified** is it
   legitimate to discuss whether the fix is worth the effort or
   should be deferred. Even then, frame the deferral around the
   identified cause, not "it's hard, let's give up".

**Companion principle:** the sibling memory
`feedback_fix_root_cause.md` says the same thing about example
code (don't fix bugs by working around them at the wrong layer).
This one is its CI / infrastructure twin.
