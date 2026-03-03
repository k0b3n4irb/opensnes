#==============================================================================
# Color Math Transparency Example
# Demonstrates shadow and tint effects using color math
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := transparency.sfc
ROM_NAME   := COLOR MATH DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background colormath

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

clean: clean-transparency
.PHONY: clean-transparency
clean-transparency:
	@rm -f res/opensnes.pic res/opensnes.pal res/opensnes.map res/opensnes.inc res/opensnes_data.as
