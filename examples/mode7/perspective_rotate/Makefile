OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := perspective_rotate.sfc
ROM_NAME := M7 PERSPECT ROTATE

USE_LIB  := 1
LIB_MODULES := console dma background hdma input mode7

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

res/ground.pc7 res/ground.mp7 res/ground.pal: res/ground.png
	@echo "[GFX] $< -> Mode 7 assets"
	@$(GFX4SNES) -p -m -M 7 -i $<

combined.asm: res/ground.pc7
