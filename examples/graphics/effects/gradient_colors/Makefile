# Gradient Colors - HDMA color gradient effect on backdrop (from PVSnesLib)

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := gradient_colors.sfc
ROM_NAME := OPENSNES GRADCOLORS

USE_LIB     := 1
LIB_MODULES := console dma background sprite hdma input math

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp Mode 1 background)
#------------------------------------------------------------------------------

res/opensnes.pic res/opensnes.pal res/opensnes.map: res/opensnes.png
	@echo "[GFX] Converting background..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

combined.asm: res/opensnes.pic

clean: clean-gradient_colors
.PHONY: clean-gradient_colors
clean-gradient_colors:
	@rm -f res/opensnes.pic res/opensnes.pal res/opensnes.map res/opensnes.inc res/opensnes_data.as
