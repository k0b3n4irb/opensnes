---
name: validation before commit
description: NEVER commit without listing impacted examples and getting user validation
type: feedback
---

NEVER commit without first listing ALL impacted examples and getting explicit user validation.

**Why:** Multiple times, library changes were committed without checking all affected examples. This caused bugs to accumulate undetected. The user had to catch them during manual testing.

**How to apply:**
1. Before ANY commit, determine which examples are impacted by the change
2. List them explicitly by path (not "the 7 reference examples")
3. Ask the user to test each one in Mesen2
4. Wait for confirmation before committing
5. If a library module changed, grep ALL example Makefiles for that module in LIB_MODULES

**For visual bugs in ported examples:**
- Compare generated ASM with the PVSnesLib original
- Check VRAM layout (tile overlaps, OBJSEL gap, tilemap conflicts)
- Use Mesen2 debugger (VRAM viewer, OAM viewer)
- Never guess or patch — find the root cause
