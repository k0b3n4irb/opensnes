---
name: Don't ask for Mesen2 manual validation when visual regression already covers it
description: For refactor-class changes where the headless visual regression baseline passes and no interactive path changed, asking the user to validate the same 2-3 examples in Mesen2 again is wasted ceremony. Only ask when Mesen2 catches something the headless suite cannot.
type: feedback
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
The user explicitly flagged on 2026-05-06 (during chantier H3): **"je remarque que l'on teste toujours les mêmes exemples"**. We had asked him to validate mode1 + mode1_bg3_priority in Mesen2 across D.2 / C.5 / P3.2 / H1 / H2 / H3 — same triage set every time. This is the same `feedback_validation_before_commit.md` rule turned against itself: always asking for the same validation is not validation, it's noise.

**Why:** the headless `run-all-tests.mjs --quick` runs visual regression on all 54 examples against pixel baselines. If a refactor only changes the bytes of `bgInitTileSet`'s call site (e.g. switching from `BG_LOAD` macro to `bgLoad` function — the H3 case), the rendered frame is byte-identical. The baseline catches this; Mesen2 sees nothing new.

**How to apply:** before asking for Mesen2 validation, ask yourself:
- Does the change affect interactive paths the headless suite doesn't exercise (button presses, animation, dynamic state, multi-frame sequences)?
- Does it introduce timing changes that visible at runtime but not in a single captured frame?
- Is the example a NEW one without a baseline?

If **yes to any** → Mesen2 validation is genuinely useful. Present a triage with "what to look for".

If **no to all** → say so explicitly: "Visual regression covers this change end-to-end (no interactive path touched, baseline matched). Mesen2 manual not needed." Commit and move on.

This sits ALONGSIDE `feedback_validation_before_commit.md`, which still applies for genuinely new behavior or interactive-path changes. The two rules together: **validate when validation adds information, not as ritual**.
