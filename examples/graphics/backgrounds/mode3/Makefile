OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode3.sfc
ROM_NAME   := MODE 3 256COLOR DEMO

USE_LIB    := 1
LIB_MODULES := console dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

# 8bpp 256-color conversion
res/bg.pic res/bg.map res/bg.pal: res/bg.bmp
	@echo "[GFX] $< -> Mode 3 assets (256 colors)"
	@$(GFX4SNES) -s 8 -o 256 -u 256 -p -m -t bmp -i $<

combined.asm: res/bg.pic res/bg.map res/bg.pal
