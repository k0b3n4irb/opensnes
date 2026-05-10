---
name: no-coauthor-trailer
description: NEVER add Co-Authored-By lines to git commit messages
type: feedback
---

Do NOT add `Co-Authored-By` trailers to commit messages. No exceptions.

**Why:** The user had to rewrite the entire git history to remove Claude from the GitHub Contributors section. AI attribution in git history is unwanted.

**How to apply:** When creating any git commit, omit the `Co-Authored-By` line entirely. Just write the commit message with no trailer.
