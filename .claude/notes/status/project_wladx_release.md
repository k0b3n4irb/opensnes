---
name: WLA-DX submodule pending release
description: compiler/wla-dx has local modifications from accepted PR — waiting for Vhelin's next release to update submodule
type: project
---

The `compiler/wla-dx` submodule has local modifications from a PR that was submitted and accepted by Vhelin (WLA-DX maintainer). We cannot update the submodule reference until Vhelin publishes a new release.

**Why:** The changes are merged upstream but not yet in a tagged release. Pointing the submodule at an intermediate commit would be fragile.

**How to apply:** Do not commit `compiler/wla-dx` submodule changes. When Vhelin publishes a new release, update the submodule to that tag.
