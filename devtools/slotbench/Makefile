# One build of the bench; run.py copies this folder per variant and passes
# OPENSNES and writes scenario.h (the BENCH_* of the variant).
TARGET   := slotbench.sfc
ROM_NAME := SLOTBENCH
USE_LIB  := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma vramqueue
VRAM_QUEUE_MAX := 64
CSRC     := main.c
ASMSRC   := sheets.asm $(wildcard slots.asm)
include $(OPENSNES)/make/common.mk
