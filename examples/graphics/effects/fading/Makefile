#==============================================================================
# Fading Effect Example
# Screen fade in/out demonstration
# Port of pvsneslib Fading example
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := fading.sfc
ROM_NAME   := FADING EFFECT

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp Mode 1 background)
#------------------------------------------------------------------------------

res/opensnes.pic res/opensnes.pal res/opensnes.map: res/opensnes.png
	@echo "[GFX] Converting background..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

combined.asm: res/opensnes.pic

clean: clean-fading
.PHONY: clean-fading
clean-fading:
	@rm -f res/opensnes.pic res/opensnes.pal res/opensnes.map res/opensnes.inc res/opensnes_data.as
