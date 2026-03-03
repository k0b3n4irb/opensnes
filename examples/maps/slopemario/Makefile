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

tiles.pic tiles.pal: tiles.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

mario_sprite.pic mario_sprite.pal: mario_sprite.bmp
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -t bmp -i $<

combined.asm: tiles.pic mario_sprite.pic

clean: clean-slopemario
.PHONY: clean-slopemario
clean-slopemario:
	@rm -f tiles.pic tiles.pal tiles.map tiles.inc tiles_data.as
	@rm -f mario_sprite.pic mario_sprite.pal mario_sprite.inc mario_sprite_data.as
