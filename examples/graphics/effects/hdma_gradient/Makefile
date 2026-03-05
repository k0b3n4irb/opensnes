# HDMA Gradient - Brightness gradient effect using HDMA (from PVSnesLib)

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := hdma_gradient.sfc
ROM_NAME := HDMA GRADIENT DEMO

USE_LIB     := 1
LIB_MODULES := console dma background sprite hdma input math

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (8bpp Mode 3 image)
#------------------------------------------------------------------------------

res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map: res/pvsneslib.png
	@echo "[GFX] Converting background..."
	@$(GFX4SNES) -s 8 -o 256 -u 256 -p -m -i $<

combined.asm: res/pvsneslib.pic

clean: clean-hdma_gradient
.PHONY: clean-hdma_gradient
clean-hdma_gradient:
	@rm -f res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map res/pvsneslib.inc res/pvsneslib_data.as
