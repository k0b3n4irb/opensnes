---
name: ASM/C constant sync
description: ASM .EQU and C #define for the same constant MUST have identical values — verify by disassembling ROM bytes
type: feedback
---

NEVER define the same constant with different values in ASM (.EQU) and C (#define).

**Why:** OBJ_SIZE16_L32 was $60 in ASM (register value) but 3 in C (index). The ASM `cmp` compared against $60 while C passed 3. The branch always failed, silently corrupting spr16addrgfx. Took hours to find because the source code *looked* correct — the bug was only visible in the ROM disassembly.

**How to apply:**
1. After ANY change to sprite_dynamic.asm constants or sprite.h constants, grep both files for the constant name and verify the values match
2. When porting PVSnesLib ASM, check if constants represent INDEX values or REGISTER values — PVSnesLib may use pre-shifted register values while our C headers use raw indices
3. When a bug defies code analysis, **disassemble the ROM bytes** (`xxd`) at the suspect instruction to verify what the assembler actually produced — don't trust source-level reading alone
4. Add `.ACCU` directives at every branch merge in hand-written ASM (WLA-DX tracking bug)
