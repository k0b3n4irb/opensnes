# Drive a sprite with the pad — Family 4 (Input), rung 4.2

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := move_sprite.sfc
ROM_NAME := MOVE SPRITE

USE_LIB     := 1
LIB_MODULES := console dma sprite input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp 32x32 sprite — shared with simple_sprite)
#------------------------------------------------------------------------------

res/sprite32.pic res/sprite32.pal: res/sprite32.png
	@echo "[GFX] Converting sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

combined.asm: res/sprite32.pic

clean: clean-move_sprite
.PHONY: clean-move_sprite
clean-move_sprite:
	@rm -f res/sprite32.pic res/sprite32.pal res/sprite32.inc res/sprite32_data.as
