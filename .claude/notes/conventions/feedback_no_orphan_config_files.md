---
name: Never leave orphan config files / duplicates in different paths
description: When adding or moving build config files, always verify they are wired up by the build infrastructure (Makefile/CI) and that no stale duplicate exists elsewhere
type: feedback
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
When adding, moving, or "improving" a build config file (Doxyfile, header.html,
.editorconfig, CMakeLists.txt, Makefile fragments, CI workflow files), you
**must** verify two things before considering the work done:

1. **The new location is actually invoked by the build.** Grep the Makefile
   and `.github/workflows/` for the file's name to confirm it is referenced
   from the path you put it in.
2. **No stale duplicate is left in another path.** If the file already
   existed somewhere else, either overwrite it in place or delete the old
   copy — never leave both versions in the tree.

**Why:** OpenSNES had a `Doxyfile` + `header.html` at the project root since
commit ac7fd0e (March 2026), with newer/better content (PROJECT_NUMBER 0.11.0,
logo, favicons) than the `docs/Doxyfile` + `docs/header.html` that existed
beforehand. But `Makefile:113` and `.github/workflows/deploy_docs.yml:22` ran
`cd docs && doxygen Doxyfile`, so the older `docs/` versions were the ones
actually used — the root files were dead weight. The user spotted it months
later and was annoyed: "c'est pas sérieux."

**How to apply:**
- After staging a config file, search for references to it: `grep -rn
  '<filename>' Makefile .github/ scripts/` and confirm at least one path
  invokes the file from where you put it.
- After moving a config file, run the affected build target (`make docs`,
  `make`, `npm run build`, etc.) and confirm it succeeds. Don't trust that
  "it should work because the paths look right."
- If you find a duplicate during a related task, flag it immediately and
  propose a fix in the same change rather than leaving it for later.
