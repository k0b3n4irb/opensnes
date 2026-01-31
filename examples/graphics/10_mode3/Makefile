# Mode 3 Example - 256-color Background
# 8bpp graphics with split DMA transfer

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mode3.sfc
ROM_NAME   := MODE3 256 COLORS

USE_LIB    := 1
LIB_MODULES := console sprite dma

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk
