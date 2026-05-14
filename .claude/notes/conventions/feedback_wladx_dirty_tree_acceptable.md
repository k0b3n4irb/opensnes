---
name: wla-dx submodule dirty working tree is acceptable, not a defect
description: The "M compiler/wla-dx" line in `git status` (CRLF normalization on tests/68000/main.s + untracked .wla-built/ + ins_tbl_gen/gen-*) is intentionally tolerated. Don't list it as a must-fix in audits.
type: feedback
originSessionId: 7cf8aa6f-20b3-4e2f-81b4-25748e91b08f
---
When `git status` from the OpenSNES root shows `m compiler/wla-dx`, the
underlying state is:

1. `tests/68000/all_instructions_test/main.s` — git CRLF normalization
   artifact (file versioned upstream with CRLF, local checkout has LF).
   Cosmetic, zero content change.
2. `.wla-built/` — CMake build dir; upstream `.gitignore` has `.wla*[ab]`
   which doesn't match `.wla-built` (ends in `t`).
3. `ins_tbl_gen/gen-*` — compiled binaries; upstream `.gitignore` covers
   only `t*.c` source files in that dir.

**Why this is acceptable**: it's been like this since the project began.
None of the three artefacts are OpenSNES-authored — they're all
consequences of upstream wla-dx's incomplete `.gitignore` plus local
build state. `make verify-toolchain` checks submodule HEAD SHA only
(by design, per `compiler/PINS.md`); working-tree cleanliness is
explicitly out of scope.

**How to apply**:
- In future SDK reviews / audits, do NOT list this as a must-fix.
- If asked about it, explain the three sources but note "user confirmed
  this is intentionally tolerated" rather than proposing a fix.
- The "Renforcer verify-toolchain pour fail sur dirty submodule" item
  (review must-fix #4 from 2026-05-10) is also off the table — same
  reason, same user signal.
- Exception: if the dirty state ever points to a *new* file class (not
  the three above), flag it — that would be a regression in upstream
  wla-dx's `.gitignore` and worth investigating.

Reference: 2026-05-10 SDK review session, user response to the
must-fix #4 listing.
