# Simple Sprite - Displays a single 32x32 sprite at center screen

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := simple_sprite.sfc
ROM_NAME := OPENSNES SIMPLESPRT

USE_LIB     := 1
LIB_MODULES := console dma sprite

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp 32x32 sprite)
#------------------------------------------------------------------------------

res/sprite32.pic res/sprite32.pal: res/sprite32.png
	@echo "[GFX] Converting sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

combined.asm: res/sprite32.pic

clean: clean-simple_sprite
.PHONY: clean-simple_sprite
clean-simple_sprite:
	@rm -f res/sprite32.pic res/sprite32.pal res/sprite32.inc res/sprite32_data.as
