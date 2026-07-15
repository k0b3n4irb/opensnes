OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := hdma_wave_table.sfc
ROM_NAME := HDMA WAVE TABLE

USE_LIB  := 1
LIB_MODULES := console dma background hdma

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# 8bpp 256-color conversion (same pipeline as graphics/backgrounds/mode3)
res/water.pic res/water.map res/water.pal: res/water.bmp
	@echo "[GFX] $< -> Mode 3 assets (256 colors)"
	@$(GFX4SNES) -s 8 -o 256 -u 256 -p -m -t bmp -i $<

combined.asm: res/water.pic res/water.map res/water.pal
