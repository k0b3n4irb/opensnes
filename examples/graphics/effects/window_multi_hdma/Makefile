OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := window_multi_hdma.sfc
ROM_NAME := WINDOW MULTI HDMA

USE_LIB  := 1
LIB_MODULES := console dma background hdma window input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

res/rings.pic res/rings.map res/rings.pal: res/rings.png
	@echo "[GFX] $< -> Mode 3 assets (256 colors)"
	@$(GFX4SNES) -s 8 -o 256 -u 256 -p -m -i $<

combined.asm: res/rings.pic
