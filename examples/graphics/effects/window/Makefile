#==============================================================================
# Window Masking Example (HDMA Triangle)
# Ported from PVSnesLib — triangle window via HDMA, toggled with A/X/B
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := window.sfc
ROM_NAME   := WINDOW HDMA DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background window hdma math

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk

# Custom gfx conversion: two BGs with different palette slots
res/bg1.pic res/bg1.map res/bg1.pal: res/bg1.png
	@echo "[GFX] $< -> res/bg1.pic/.map/.pal"
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

res/bg2.pic res/bg2.map res/bg2.pal: res/bg2.png
	@echo "[GFX] $< -> res/bg2.pic/.map/.pal (palette slot 1)"
	@$(GFX4SNES) -s 8 -o 16 -u 16 -e 1 -p -m -i $<

# Ensure assets are converted before assembly (data.asm .incbin references them)
combined.asm: res/bg1.pic res/bg1.map res/bg1.pal res/bg2.pic res/bg2.map res/bg2.pal

# Extended clean
clean: clean-window
.PHONY: clean-window
clean-window:
	@rm -f res/bg1.pic res/bg1.pal res/bg1.map res/bg1.inc res/bg1_data.as
	@rm -f res/bg2.pic res/bg2.pal res/bg2.map res/bg2.inc res/bg2_data.as
