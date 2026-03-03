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

tilesMario.pic tilesMario.pal: tilesMario.png
	@echo "[GFX] Converting tileset..."
	@$(GFX4SNES) -s 8 -o 48 -u 16 -p -m -i $<

mario.pic mario.pal: mario.png
	@echo "[GFX] Converting mario sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

goomba.pic goomba.pal: goomba.png
	@echo "[GFX] Converting goomba sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

koopatroopa.pic koopatroopa.pal: koopatroopa.png
	@echo "[GFX] Converting koopatroopa sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

# Ensure graphics are converted before assembly
combined.asm: tilesMario.pic mario.pic goomba.pic koopatroopa.pic

# Extended clean
clean: clean-mapandobjects
.PHONY: clean-mapandobjects
clean-mapandobjects:
	@rm -f tilesMario.pic tilesMario.pal tilesMario.map tilesMario.inc tilesMario_data.as
	@rm -f mario.pic mario.pal mario.inc mario_data.as
	@rm -f goomba.pic goomba.pal goomba.inc goomba_data.as
	@rm -f koopatroopa.pic koopatroopa.pal koopatroopa.inc koopatroopa_data.as
