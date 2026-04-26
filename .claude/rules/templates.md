---
paths:
  - "templates/**/*"
  - "make/common.mk"
---

# Templates & Build System Rules

## Key Files

- `crt0.asm` — Startup code, NMI handler, system variables, data init copy loop
- `hdr.asm` / `hdr_hirom.asm` / `hdr_sa1.asm` — ROM headers, memory maps, interrupt vectors
- `runtime.asm` — Math/utility routines used by compiled C code
- `memmap.inc` / `memmap_hirom.inc` / `memmap_sa1.inc` — WLA-DX memory map definitions
- `make/common.mk` — Universal build rules (478 lines), included by every example

## Memory Layout (LoROM)

```
$00:0000-004F   Compiler registers (tcc__r0-r10h) — FORCE section bank $00
$00:0080        NMI callback pointer
$00:0100+       System variables (vblank_flag, oam_update_flag, frame_count)
$00:0100 page   tcc__nmi_registers — DP isolation for NMI callbacks
$7E:0300-051F   OAM buffer (544 bytes: 128 sprites × 4 + 32 hi-table)
$7E:0400-1FFF   Available WRAM for C code
$8000-FFFF      ROM code/data (32KB per bank)
```

## Data Initialization

Initialized C data flows: `.data_init` sections in ROM → DMA copied to WRAM at boot by crt0.asm `CopyInitData`.

ROM format: `[target_addr:2][size:2][data:N]...` with sentinel `target_addr = 0`.

CRITICAL: `data_init_end.o` MUST be linked last — it provides the sentinel.

## Linker Object Order

```
combined.obj (crt0+runtime+ASM) → C objects → library objects → data_init_end.o
```

## Example Makefile Pattern

```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := game.sfc
ROM_NAME := "GAME NAME           "   # exactly 21 chars
USE_LIB  := 1
LIB_MODULES := console sprite dma input   # only link what you need
CSRC := main.c
# USE_FASTROM := 1            # optional: ~33% faster ROM access
# USE_HIROM := 1              # optional: HiROM layout
# USE_SA1 := 1                # optional: SA-1 enhancement chip
include $(OPENSNES)/make/common.mk
```

## After Modifying Templates

This is a Class A change:
1. `make clean && make` — REQUIRED (stale crt0 objects cause phantom failures)
2. Full test suite — all 212 checks
3. Mesen2 validation on ALL affected examples
4. If modifying NMI handler: consult `.claude/rules/nmi_audit.md` first
