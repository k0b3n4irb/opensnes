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

USE_SNESMOD    := 1
SOUNDBANK_SRC  := res/overworld.it res/jump_sfx.it

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (custom rules since tiles and sprites need different flags)
#------------------------------------------------------------------------------

res/tiles.pic res/tiles.pal: res/tiles.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

res/mario_sprite.pic res/mario_sprite.pal: res/mario_sprite.png
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

# Ensure graphics are converted before assembly
combined.asm: res/tiles.pic res/mario_sprite.pic

# Extended clean
clean: clean-likemario
.PHONY: clean-likemario
clean-likemario:
	@rm -f res/tiles.pic res/tiles.pal res/tiles.map res/tiles.inc res/tiles_data.as
	@rm -f res/mario_sprite.pic res/mario_sprite.pal res/mario_sprite.inc res/mario_sprite_data.as
