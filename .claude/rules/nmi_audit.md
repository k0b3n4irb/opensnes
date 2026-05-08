---
paths:
  - "templates/crt0.asm"
  - "templates/**/*"
  - "lib/source/*.asm"
---

# NMI/VBlank Audit Rules

CRITICAL: Consult this file before ANY change to crt0.asm or NMI-related code.

## NMI Execution Order

The NMI handler runs at VBlank (~60Hz) in this exact order:

1. **OAM DMA** — VBlank-critical (VRAM write)
2. **Tilemap DMA** — VBlank-critical (VRAM write)
3. **BG scroll sync** — VBlank-critical (PPU register write)
4. **User callback** — not VBlank-critical
5. **Joypad read** — not VBlank-critical ($4218/$421A readable anytime)
6. **Mouse read** — not VBlank-critical ($4016/$4017 serial)
7. **Super Scope** — not VBlank-critical (PPU latch + $421A)

Steps 1-3 MUST complete before VBlank ends (~4KB DMA budget). Do NOT reorder them.

## Handshake Protocol

```
Main thread: sets vblank_flag=1 via WaitForVBlank ("I'm ready")
NMI handler: checks flag → if 0 = lag frame (skip work), if 1 = do work
NMI handler: clears flag=0 when done ("acknowledged")
Main thread: wakes from WAI, sees flag=0, proceeds
```

Lag frames increment `frame_count` but skip VRAM work. This is intentional.

## WRAM Data Port — CRITICAL

**$2180-$2183 is NOT safe in NMI code.**

Main-thread code writes multi-byte sequences to $2180 with address set via $2181-$2183.
If NMI fires mid-sequence and touches $2180-$2183, the address/data will be **corrupted silently** when the main thread resumes.

## DP Isolation

NMI uses `tcc__nmi_registers` (page-aligned, != 0) as its direct page.
All dp-relative accesses in NMI callback C code hit NMI registers, NOT main thread registers. No save/restore needed — saves 260 cycles.

## Audit Checklist (before modifying NMI)

- [ ] VBlank-critical steps (OAM, tilemap, scroll) remain first
- [ ] DMA budget stays within ~4KB per frame
- [ ] No WRAM data port ($2180-$2183) access in NMI path
- [ ] Handshake protocol (vblank_flag) unchanged
- [ ] DP isolation preserved (tcc__nmi_registers)
- [ ] FASTROM variant (jml.l FastNmi) updated if applicable
- [ ] Every example still passes visual regression (run the full `--quick` suite — counts drift; the gate is "all green", not a hard-coded number)
