# Parallax Scrolling - HDMA-based multi-speed horizontal scroll (from PVSnesLib)

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := parallax_scrolling.sfc
ROM_NAME := OPENSNES PARALLAX

USE_LIB     := 1
LIB_MODULES := console dma background sprite hdma input math

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# Custom gfx conversion: need -m (tilemap) and -o 32 -u 16 (tile offset/unique)
# gfx4snes outputs to the same directory as the input file
res/back.pic res/back.map res/back.pal: res/back.png
	@echo "[GFX] $< -> res/back.pic/.map/.pal"
	@$(GFX4SNES) -s 8 -o 32 -u 16 -p -m -i $<

# Ensure assets are converted before assembly (data.asm .incbin references them)
combined.asm: res/back.pic res/back.map res/back.pal
