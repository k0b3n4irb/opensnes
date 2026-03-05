#==============================================================================
# Slope Mario - Platformer with slope collision
#==============================================================================
# Demonstrates: objCollidMapWithSlopes for diagonal terrain collision,
#               map engine scrolling, dynamic sprites, player physics.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := slopemario.sfc
ROM_NAME    := OPENSNES SLOPMARIO

CSRC        := main.c mario.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

res/tiles.pic res/tiles.pal: res/tiles.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

res/mario_sprite.pic res/mario_sprite.pal: res/mario_sprite.png
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

combined.asm: res/tiles.pic res/mario_sprite.pic

clean: clean-slopemario
.PHONY: clean-slopemario
clean-slopemario:
	@rm -f res/tiles.pic res/tiles.pal res/tiles.map res/tiles.inc res/tiles_data.as
	@rm -f res/mario_sprite.pic res/mario_sprite.pal res/mario_sprite.inc res/mario_sprite_data.as
