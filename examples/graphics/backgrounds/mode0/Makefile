OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode0.sfc
ROM_NAME   := MODE 0 4BG DEMO

USE_LIB    := 1
LIB_MODULES := console dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

# Mode 0: 2bpp (4-color) per BG
res/bg0.pic res/bg0.map res/bg0.pal: res/bg0.bmp
	@echo "[GFX] $< -> BG0 (2bpp)"
	@$(GFX4SNES) -s 8 -o 4 -u 4 -e 0 -p -t bmp -m -i $<

res/bg1.pic res/bg1.map res/bg1.pal: res/bg1.bmp
	@echo "[GFX] $< -> BG1 (2bpp)"
	@$(GFX4SNES) -s 8 -o 4 -u 4 -e 0 -p -t bmp -m -i $<

res/bg2.pic res/bg2.map res/bg2.pal: res/bg2.bmp
	@echo "[GFX] $< -> BG2 (2bpp)"
	@$(GFX4SNES) -s 8 -o 4 -u 4 -e 0 -p -t bmp -m -i $<

res/bg3.pic res/bg3.map res/bg3.pal: res/bg3.bmp
	@echo "[GFX] $< -> BG3 (2bpp, 8-color pal)"
	@$(GFX4SNES) -s 8 -o 8 -u 4 -e 0 -p -t bmp -m -i $<

combined.asm: res/bg0.pic res/bg1.pic res/bg2.pic res/bg3.pic
