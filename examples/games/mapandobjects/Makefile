#==============================================================================
# Map and Objects - Scrolling platformer with object engine
#==============================================================================
# Demonstrates: map engine scrolling, object engine with physics/collision,
#               dynamic sprites, enemy AI (goomba, koopa troopa).
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := mapandobjects.sfc
ROM_NAME    := OPENSNES MAPNOBJ

CSRC        := main.c mario.c goomba.c koopatroopa.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------

res/tilesMario.pic res/tilesMario.pal: res/tilesMario.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 48 -u 16 -p -m -i $<

res/mario.pic res/mario.pal: res/mario.png
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

res/goomba.pic res/goomba.pal: res/goomba.png
	@echo "[GFX] Converting goomba sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

res/koopatroopa.pic res/koopatroopa.pal: res/koopatroopa.png
	@echo "[GFX] Converting koopatroopa sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

# Ensure graphics are converted before assembly
combined.asm: res/tilesMario.pic res/mario.pic res/goomba.pic res/koopatroopa.pic

# Extended clean
clean: clean-mapandobjects
.PHONY: clean-mapandobjects
clean-mapandobjects:
	@rm -f res/tilesMario.pic res/tilesMario.pal res/tilesMario.map res/tilesMario.inc res/tilesMario_data.as
	@rm -f res/mario.pic res/mario.pal res/mario.inc res/mario_data.as
	@rm -f res/goomba.pic res/goomba.pal res/goomba.inc res/goomba_data.as
	@rm -f res/koopatroopa.pic res/koopatroopa.pal res/koopatroopa.inc res/koopatroopa_data.as
