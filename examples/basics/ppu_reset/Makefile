# ppuReset() — Basics: the next screen starts from a clean PPU

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := ppu_reset.sfc
ROM_NAME := PPU RESET

USE_LIB     := 1
LIB_MODULES := console dma text background sprite colormath mosaic window mode7 hdma ppu_reset

CSRC := main.c

include $(OPENSNES)/make/common.mk
