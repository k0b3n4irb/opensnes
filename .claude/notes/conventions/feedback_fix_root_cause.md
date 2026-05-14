---
name: Fix root cause not symptoms
description: NEVER fix bugs with workarounds in example code — always find and fix the systemic root cause in compiler/library/templates/build
type: feedback
---

When a bug appears in an example, NEVER add workarounds to the example code. Think globally across all layers: compiler, library, templates, makefile.

**Why:** During the dynamic_metasprite debugging, multiple attempts were made to "fix" the example (change BG mode, relocate VRAM addresses, add extra dmaCopyVram, add extern variable writes). Each one hid the real bug deeper. The actual root cause was a single ASM constant (`OBJ_SIZE16_L32 = $60`) mismatching the C header (`= 3`) — a library-level issue, not an example issue.

**How to apply:**
1. When a bug appears, first ask: "Is this a compiler, library, template, or build issue?"
2. Compare with PVSnesLib original side-by-side in Mesen2 BEFORE attempting any fix
3. If source code looks correct but behavior is wrong, disassemble the ROM bytes
4. NEVER commit a "fix" that's actually a workaround in example code
5. When stuck after 2-3 failed attempts, step back and verify assembled ROM bytes instead of re-reading source code
