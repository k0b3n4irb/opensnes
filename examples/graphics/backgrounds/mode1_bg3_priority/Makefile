# Mode 1 BG3 High Priority Example
# Demonstrates BG3 as a HUD layer overlaying BG1+BG2 in Mode 1

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode1_bg3_priority.sfc
ROM_NAME   := MODE1 BG3 PRIORITY

USE_LIB    := 1
LIB_MODULES := console sprite dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (Mode 1: BG1/BG2 4bpp, BG3 2bpp)
#------------------------------------------------------------------------------

res/BG1.pic res/BG1.pal res/BG1.map: res/BG1.png
	@echo "[GFX] Converting BG1..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -e 2 -p -m -i $<

res/BG2.pic res/BG2.pal res/BG2.map: res/BG2.png
	@echo "[GFX] Converting BG2..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -e 4 -p -m -i $<

res/BG3.pic res/BG3.pal res/BG3.map: res/BG3.png
	@echo "[GFX] Converting BG3..."
	@$(GFX4SNES) -g -s 8 -o 4 -u 16 -e 0 -p -m -i $<

combined.asm: res/BG1.pic res/BG2.pic res/BG3.pic

clean: clean-mode1_bg3_priority
.PHONY: clean-mode1_bg3_priority
clean-mode1_bg3_priority:
	@rm -f res/BG1.pic res/BG1.pal res/BG1.map res/BG1.inc res/BG1_data.as
	@rm -f res/BG2.pic res/BG2.pal res/BG2.map res/BG2.inc res/BG2_data.as
	@rm -f res/BG3.pic res/BG3.pal res/BG3.map res/BG3.inc res/BG3_data.as
