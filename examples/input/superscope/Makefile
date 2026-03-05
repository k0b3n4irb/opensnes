#==============================================================================
# SNES Super Scope Example Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := superscope.sfc
ROM_NAME   := SUPERSCOPE DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console input sprite dma text background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

# Sprites: 4bpp 32x32, 48-color palette (3 sub-palettes)
res/sprites.pic res/sprites.pal: res/sprites.png
	@echo "[GFX] Converting sprites..."
	@$(GFX4SNES) -s 32 -o 48 -u 16 -p -i $<

# Aim adjustment target: 2bpp background with tilemap
res/aim_adjust_target.pic res/aim_adjust_target.pal res/aim_adjust_target.map: res/aim_adjust_target.png
	@echo "[GFX] Converting aim target background..."
	@$(GFX4SNES) -s 8 -o 4 -u 4 -p -m -i $<

combined.asm: res/sprites.pic res/aim_adjust_target.pic

clean: clean-superscope
.PHONY: clean-superscope
clean-superscope:
	@rm -f res/sprites.pic res/sprites.pal res/sprites.inc res/sprites_data.as
	@rm -f res/aim_adjust_target.pic res/aim_adjust_target.pal res/aim_adjust_target.map
	@rm -f res/aim_adjust_target.inc res/aim_adjust_target_data.as
