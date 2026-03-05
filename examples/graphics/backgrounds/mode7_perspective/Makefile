# Mode 7 Perspective - F-Zero style ground perspective with HDMA sky
# Based on PVSnesLib Mode7Perspective by mills32 / alekmaul

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode7_perspective.sfc
ROM_NAME := OPENSNES MODE7 PERSP

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mode7

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

# Sky: standard 4bpp background
res/sky.pic res/sky.pal res/sky.map: res/sky.png
	@echo "[GFX] Converting sky background..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -m -p -i $<

# Ground: Mode 7 format (.pc7/.mp7)
res/ground.pc7 res/ground.mp7 res/ground.pal: res/ground.png
	@echo "[GFX] Converting Mode 7 ground..."
	@$(GFX4SNES) -p -m -M 7 -i $<

combined.asm: res/sky.pic res/ground.pc7

clean: clean-mode7_perspective
.PHONY: clean-mode7_perspective
clean-mode7_perspective:
	@rm -f res/sky.pic res/sky.pal res/sky.map res/sky.inc res/sky_data.as
	@rm -f res/ground.pc7 res/ground.mp7 res/ground.pal res/ground.inc res/ground_data.as
