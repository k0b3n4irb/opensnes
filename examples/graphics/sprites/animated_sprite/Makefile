# Animated Sprite — Direction-based sprite animation with flip
OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := animated_sprite.sfc
ROM_NAME   := ANIMATED SPRITE

USE_LIB    := 1
LIB_MODULES := console sprite dma input

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp 16x16 sprite)
#------------------------------------------------------------------------------

res/sprites.pic res/sprites.pal: res/sprites.png
	@echo "[GFX] Converting sprites..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

combined.asm: res/sprites.pic

clean: clean-animated_sprite
.PHONY: clean-animated_sprite
clean-animated_sprite:
	@rm -f res/sprites.pic res/sprites.pal res/sprites.inc res/sprites_data.as
