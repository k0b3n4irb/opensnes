#==============================================================================
# LikeMario - Side-scrolling platformer example
#==============================================================================
# Demonstrates: tile-based scrolling, sprite animation, physics, collision,
#               camera tracking, and SNESMOD music playback.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := likemario.sfc
ROM_NAME    := OPENSNES LIKEMARIO

CSRC        := main.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background

# SNESMOD audio (disabled for dichotomy test)
#USE_SNESMOD    := 1
#SOUNDBANK_SRC  := overworld.it

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (custom rules since tiles and sprites need different flags)
#------------------------------------------------------------------------------

tiles.pic tiles.pal: tiles.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

mario_sprite.pic mario_sprite.pal: mario_sprite.bmp
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -t bmp -i $<

# Ensure graphics are converted before assembly
combined.asm: tiles.pic mario_sprite.pic

# Extended clean
clean: clean-likemario
.PHONY: clean-likemario
clean-likemario:
	@rm -f tiles.pic tiles.pal tiles.map mario_sprite.pic mario_sprite.pal
