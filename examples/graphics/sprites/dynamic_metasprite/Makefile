# Dynamic Metasprite Engine Example Makefile

OPENSNES := $(shell cd ../../../.. && pwd)
TARGET   := dynamic_metasprite.sfc
ROM_NAME := DYN_METASPRITE_DEMO

CSRC     := main.c
ASMSRC   := data.asm

USE_LIB  := 1
LIB_MODULES := console sprite sprite_dynamic sprite_dynamic_meta sprite_lut dma background text input

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (metasprite tile sheets)
#------------------------------------------------------------------------------

res/spritehero32.pic res/spritehero32.pal: res/spritehero32.png
	@echo "[GFX] Converting 32x32 metasprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -T -X 64 -Y 64 -P 2 -i $<

res/spritehero16.pic res/spritehero16.pal: res/spritehero16.png
	@echo "[GFX] Converting 16x16 metasprite..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -T -X 32 -Y 48 -P 2 -i $<

res/spritehero8.pic res/spritehero8.pal: res/spritehero8.png
	@echo "[GFX] Converting 8x8 metasprite..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -T -X 16 -Y 16 -P 2 -i $<

combined.asm: res/spritehero32.pic res/spritehero16.pic res/spritehero8.pic

clean: clean-dynamic_metasprite
.PHONY: clean-dynamic_metasprite
clean-dynamic_metasprite:
	@rm -f res/spritehero32.pic res/spritehero32.pal res/spritehero32.inc res/spritehero32_data.as
	@rm -f res/spritehero16.pic res/spritehero16.pal res/spritehero16.inc res/spritehero16_data.as
	@rm -f res/spritehero8.pic res/spritehero8.pal res/spritehero8.inc res/spritehero8_data.as
