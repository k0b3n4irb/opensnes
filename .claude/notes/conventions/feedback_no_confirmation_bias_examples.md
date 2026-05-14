---
name: Don't fix audit findings by creating examples that demonstrate your own changes
description: When an audit lists "missing examples" for code I just wrote, default to closing the finding as an audit artefact unless a real user need is documented. Creating examples to validate my own framework decisions is confirmation-bias engineering, not user-driven design.
type: feedback
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
The user flagged on 2026-05-06 (during audit Phase 2): **"tu crée des nouveaux exemples qui vont dans le sens de tes changements"**. I had just authored the framework trilogy D.1/D.2/D.3 and identified "missing examples" as audit issue M7 — then proposed creating three new examples whose only purpose was to demonstrate the trilogy I had designed.

This is a real anti-pattern:
- The "missing example" gap was invented by me, in an audit of code I wrote, to be filled by examples that prove my own framework was right.
- No actual user had asked for an asset-table demo, a scene-swap demo, or a scene-shared-state demo.
- The questions the proposed examples "answered" were either already covered by existing examples (e.g. shared state via globals — `scene_stack` already does this with `counter_value`) or had 1-line doc answers (swap = pop+push).

**How to apply:** when an audit lists "missing examples" or "under-demonstrated patterns", before proposing new examples ask:

1. Does any **existing user / external project** ask for the pattern? If no, the gap is speculative.
2. Does an **existing example** already show it (even if the docs don't point at it)? Often yes.
3. Can a **1-line doc note** cover it? If so, prefer that over a new file.
4. Is the proposed example **driven by a real workflow** (level select that needs assets, mid-game pause that needs swap) or by **the framework's API surface** ("we should show off the typed form")?

If 1+2+3 close the gap, **declare the audit finding an artefact and skip the example**. Don't invent demand.

Real-world signal: if the new example would have to invent its own gameplay scaffold to exercise the framework feature, that's a strong "you're building demand for your own API" smell.

This sits alongside `feedback_no_compromise_root_cause.md`: the inverse pattern. There, stop accepting "this is fine" and dig deeper. Here, stop manufacturing work to validate your own decisions.
