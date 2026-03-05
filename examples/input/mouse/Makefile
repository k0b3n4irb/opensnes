#==============================================================================
# SNES Mouse Example Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mouse.sfc
ROM_NAME   := MOUSE DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console input sprite dma text background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp cursor sprite)
#------------------------------------------------------------------------------

res/cursor.pic res/cursor.pal: res/cursor.png
	@echo "[GFX] Converting cursor sprite..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -i $<

combined.asm: res/cursor.pic

clean: clean-mouse
.PHONY: clean-mouse
clean-mouse:
	@rm -f res/cursor.pic res/cursor.pal res/cursor.inc res/cursor_data.as
