# Dynamic Sprite Engine Example Makefile

OPENSNES := $(shell cd ../../../.. && pwd)
TARGET   := dynamic_sprite.sfc
ROM_NAME := DYNAMIC_SPRITE_DEMO

CSRC     := main.c
ASMSRC   := data.asm

USE_LIB  := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma background input

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp 32x32 sprite)
#------------------------------------------------------------------------------

res/sprite32.pic res/sprite32.pal: res/sprite32.png
	@echo "[GFX] Converting 32x32 sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

combined.asm: res/sprite32.pic

clean: clean-dynamic_sprite
.PHONY: clean-dynamic_sprite
clean-dynamic_sprite:
	@rm -f res/sprite32.pic res/sprite32.pal res/sprite32.inc res/sprite32_data.as
