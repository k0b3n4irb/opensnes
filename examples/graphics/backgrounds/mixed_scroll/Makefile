#==============================================================================
# Mode 1 Mixed Scroll Example
# Two Mode 1 backgrounds: BG1 (shader) auto-scrolls, BG2 (pvsneslib logo) fixed
# Port of PVSnesLib Mode1MixedScroll example
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mixed_scroll.sfc
ROM_NAME   := MODE1 MIXED SCROLL

USE_LIB    := 1
LIB_MODULES := console sprite dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Custom graphics conversion (different flags per image)
#------------------------------------------------------------------------------

# Shader: 4bpp, palette entry 1 (tilemap references palette slot 1)
res/shader.pic res/shader.pal res/shader.map: res/shader.png
	@echo "[GFX] $< (palette entry 1)"
	@$(GFX4SNES) -s 8 -o 16 -e 1 -m -p -i $<

# PVSnesLib logo: 4bpp, palette entry 0 (default palette slot)
res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map: res/pvsneslib.png
	@echo "[GFX] $< (palette entry 0)"
	@$(GFX4SNES) -s 8 -o 16 -m -p -i $<

# Extended clean
clean: clean-mixed_scroll
.PHONY: clean-mixed_scroll
clean-mixed_scroll:
	@rm -f res/shader.pic res/shader.pal res/shader.map res/shader.inc res/shader_data.as
	@rm -f res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map res/pvsneslib.inc res/pvsneslib_data.as
