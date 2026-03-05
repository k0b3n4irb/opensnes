#==============================================================================
# Transparency Example
# Color addition between two BG layers (landscape + scrolling clouds)
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := transparency.sfc
ROM_NAME   := TRANSPARENCY DEMO

USE_LIB    := 1
LIB_MODULES := console sprite dma background colormath

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

# Landscape: 4bpp (16 colors), palette entry 1 in tilemap (BMP input, same flags as PVSnesLib)
res/backgrounds.pic res/backgrounds.pal res/backgrounds.map: res/backgrounds.bmp
	@echo "[GFX] Converting landscape (4bpp)..."
	@$(GFX4SNES) -a -s 8 -u 16 -e 1 -p -m -t bmp -i $<

# Clouds: 2bpp (4 colors) (BMP input, same flags as PVSnesLib)
res/clouds.pic res/clouds.pal res/clouds.map: res/clouds.bmp
	@echo "[GFX] Converting clouds (2bpp)..."
	@$(GFX4SNES) -s 8 -o 2 -u 4 -p -m -t bmp -i $<

combined.asm: res/backgrounds.pic res/clouds.pic

clean: clean-transparency
.PHONY: clean-transparency
clean-transparency:
	@rm -f res/backgrounds.pic res/backgrounds.pal res/backgrounds.map res/backgrounds.inc res/backgrounds_data.as
	@rm -f res/clouds.pic res/clouds.pal res/clouds.map res/clouds.inc res/clouds_data.as
	@rm -f res/opensnes.pic res/opensnes.pal res/opensnes.map res/opensnes.inc res/opensnes_data.as
