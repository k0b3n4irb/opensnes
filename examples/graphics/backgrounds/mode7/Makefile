# Mode 7 - Demonstrates Mode 7 rotation and scaling with joypad input

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode7.sfc
ROM_NAME := OPENSNES MODE7

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mode7

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (Mode 7: 8bpp tiles + 128x128 tilemap)
#------------------------------------------------------------------------------

res/mode7bg.pc7 res/mode7bg.mp7 res/mode7bg.pal: res/mode7bg.png
	@echo "[GFX] Converting Mode 7 background..."
	@$(GFX4SNES) -p -m -M 7 -i $<

combined.asm: res/mode7bg.pc7

clean: clean-mode7
.PHONY: clean-mode7
clean-mode7:
	@rm -f res/mode7bg.pc7 res/mode7bg.mp7 res/mode7bg.pal res/mode7bg.inc res/mode7bg_data.as
