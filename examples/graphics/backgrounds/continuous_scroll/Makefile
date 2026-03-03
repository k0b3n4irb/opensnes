# Continuous Scroll Example
# Multi-layer parallax with sprite

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := continuous_scroll.sfc
ROM_NAME   := CONTINUOUS SCROLL

USE_LIB    := 1
LIB_MODULES := console sprite input background dma

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

# BG1: 4bpp background, palette entry 2, 32x32 page map for scrolling
res/BG1.pic res/BG1.pal res/BG1.map: res/BG1.png
	@echo "[GFX] Converting BG1..."
	@$(GFX4SNES) -y -s 8 -o 16 -u 16 -e 2 -p -m -i $<

# BG2: 4bpp background, palette entry 4, high priority, 32x32 page map
res/BG2.pic res/BG2.pal res/BG2.map: res/BG2.png
	@echo "[GFX] Converting BG2..."
	@$(GFX4SNES) -g -y -s 8 -o 16 -u 16 -e 4 -p -m -i $<

# BG3: 2bpp HUD overlay, high priority
res/BG3.pic res/BG3.pal res/BG3.map: res/BG3.png
	@echo "[GFX] Converting BG3..."
	@$(GFX4SNES) -g -s 8 -o 4 -u 16 -e 0 -p -m -i $<

# Character sprite: 4bpp 16x16
res/character.pic res/character.pal: res/character.png
	@echo "[GFX] Converting character sprite..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

combined.asm: res/BG1.pic res/BG2.pic res/BG3.pic res/character.pic

clean: clean-continuous_scroll
.PHONY: clean-continuous_scroll
clean-continuous_scroll:
	@rm -f res/BG1.pic res/BG1.pal res/BG1.map res/BG1.inc res/BG1_data.as
	@rm -f res/BG2.pic res/BG2.pal res/BG2.map res/BG2.inc res/BG2_data.as
	@rm -f res/BG3.pic res/BG3.pal res/BG3.map res/BG3.inc res/BG3_data.as
	@rm -f res/character.pic res/character.pal res/character.inc res/character_data.as
