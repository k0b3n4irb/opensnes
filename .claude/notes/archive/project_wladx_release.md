---
name: WLA-DX submodule pending release
description: compiler/wla-dx has local modifications from accepted PR — waiting for Vhelin's next release to update submodule
type: project
---

> **📁 ARCHIVED 2026-07-26 — RESOLVED.** `compiler/wla-dx` is now pinned to
> the **v10.7 release tag** (`91c52b1f`) with **zero local patches** (see
> `compiler/PINS.md`). The wait is over and the guidance below ("do not
> commit `compiler/wla-dx` submodule changes") is **superseded** — the
> submodule was advanced to v10.7 on 2026-07-25. Later wla-dx work (the
> SPAN regression on master, filed upstream as vhelin/wla-dx#729) lives in
> `.claude/notes/tech/wla_span_*`.

The `compiler/wla-dx` submodule has local modifications from a PR that was submitted and accepted by Vhelin (WLA-DX maintainer). We cannot update the submodule reference until Vhelin publishes a new release.

**Why:** The changes are merged upstream but not yet in a tagged release. Pointing the submodule at an intermediate commit would be fragile.

**How to apply:** Do not commit `compiler/wla-dx` submodule changes. When Vhelin publishes a new release, update the submodule to that tag.
