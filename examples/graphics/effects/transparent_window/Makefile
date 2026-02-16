#==============================================================================
# Transparent Window Example
# Ported from PVSnesLib — semi-transparent rectangle using color math + HDMA
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := transparent_window.sfc
ROM_NAME   := TRANSPARENT WINDOW

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma background window colormath hdma math

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk

# Custom gfx conversion
res/background.pic res/background.map res/background.pal: res/background.png
	@echo "[GFX] $< -> res/background.pic/.map/.pal"
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

# Ensure assets are converted before assembly
combined.asm: res/background.pic res/background.map res/background.pal
