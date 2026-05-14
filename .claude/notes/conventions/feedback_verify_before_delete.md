---
name: verify before deleting references
description: Always verify file existence AND cross-check intent before removing references from docs
type: feedback
---

Never remove a reference to a file without double-checking BOTH that the file doesn't exist AND that removing it is the right action. In the CONTRIBUTING.md incident, the file existed at the project root but was incorrectly removed from mainpage.md.

**Why:** User had to catch the mistake. Removing valid references degrades documentation quality.

**How to apply:** When fixing broken links, verify each target file exists before removing the reference. If unsure, keep the reference and flag it for review rather than deleting.
