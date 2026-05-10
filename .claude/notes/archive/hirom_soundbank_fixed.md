---
name: HiROM soundbank linker error
description: WLA-DX linker fails with "ROMBANKMAPs don't match" when combining HiROM + SNESMOD soundbank. Needs investigation.
type: project
---

## Problem (2026-03-21)

Building `snesmod_music_hirom` fails at link time:
```
OBTAIN_ROMBANKS: Using the biggest selected amount of ROM banks (8).
OBTAIN_ROMBANKMAP: ROMBANKMAPs don't match.
```

**Why:** The `combined.obj` (which includes the soundbank ASM) has a different internal header value (byte[5]=$C6) compared to other objects (byte[5]=$80). This appears to be related to how WLA-DX encodes the ROMBANKMAP when the assembled file contains `.BANK` directives with FORCE sections.

**How to apply:**
- `snesmod_music_hirom` example exists but doesn't build
- All LoROM examples (including large soundbank >32KB) work correctly
- HiROM without audio works correctly (`hirom_demo` builds fine)
- The issue is specifically the combination of HiROM MEMORYMAP + `.BANK N` + `.ORG $8000` + FORCE sections from the soundbank

## Investigation needed

1. Check WLA-DX source code for how it handles ROMBANKMAP in object files
2. Check if `.BANK` directives inside a file that already has a MEMORYMAP create a conflict
3. Try assembling the soundbank as a SEPARATE object file (not concatenated into combined.asm)
4. Try using SUPERFREE instead of FORCE for soundbank sections in HiROM
5. Compare with PVSnesLib's approach — they use `-b 3` (bank 3, last bank) for HiROM

## Current workarounds attempted

- ROMBANKS increased from 4 to 8 — didn't fix it
- `-i` flag removed from smconv (causes no split) — didn't fix it
- `.ORG $8000` via sed for HiROM — might be part of the problem

## What works

- LoROM soundbank >32KB: split across 2 banks with FORCE sections ✓
- LoROM single-bank soundbank ✓
- HiROM without audio ✓
