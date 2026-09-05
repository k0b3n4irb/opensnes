# DSP-1 ground — Super Mario Kart style Mode 7 floor: the per-scanline
# matrices come from the DSP-1 Raster command instead of a precomputed table.
# Assets reused from mode7/perspective (PVSnesLib Mode7Perspective, mills32 / alekmaul).

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := dsp1_ground.sfc
ROM_NAME := DSP1 GROUND

USE_LIB     := 1
USE_DSP1    := 1
LIB_MODULES := console dma background input mode7 hdma   # dsp1 auto-added by USE_DSP1

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (same recipes as mode7/perspective)
#------------------------------------------------------------------------------

# Sky: standard 4bpp background, 16 colours shared with the ground palette
res/sky.pic res/sky.pal res/sky.map: res/sky.png
	@echo "[GFX] Converting sky background..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -m -p -i $<

# Ground: Mode 7 format (.pc7/.mp7)
res/ground.pc7 res/ground.mp7 res/ground.pal: res/ground.png
	@echo "[GFX] Converting Mode 7 ground..."
	@$(GFX4SNES) -p -m -M 7 -i $<

combined.asm: res/sky.pic res/ground.pc7

clean: clean-mode7_dsp1_ground
.PHONY: clean-mode7_dsp1_ground
clean-mode7_dsp1_ground:
	@rm -f res/sky.pic res/sky.pal res/sky.map res/sky.inc res/sky_data.as
	@rm -f res/ground.pc7 res/ground.mp7 res/ground.pal res/ground.inc res/ground_data.as
