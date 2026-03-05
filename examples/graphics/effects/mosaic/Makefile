# Mosaic - Demonstrates mosaic shading effect with fade transitions

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mosaic.sfc
ROM_NAME := OPENSNES MOSAIC

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mosaic

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

clean: clean-mosaic
.PHONY: clean-mosaic
clean-mosaic:
	@rm -f res/opensnes.pic res/opensnes.pal res/opensnes.map res/opensnes.inc res/opensnes_data.as
