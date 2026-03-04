OPENSNES := $(shell cd ../../../.. && pwd)
TARGET   := metasprite.sfc
ROM_NAME := METASPRITE_DEMO

CSRC     := main.c
ASMSRC   := data.asm

USE_LIB  := 1
LIB_MODULES := console sprite dma text text4bpp background input

include $(OPENSNES)/make/common.mk

# Custom gfx rules for metasprite tile layout (-T transposes for SNES OBJ grid)
res/spritehero32.pic res/spritehero32.pal: res/spritehero32.png
	$(GFX4SNES) -s 32 -o 16 -u 16 -p -T -X 64 -Y 64 -P 2 -i $<

res/spritehero16.pic res/spritehero16.pal: res/spritehero16.png
	$(GFX4SNES) -s 16 -o 16 -u 16 -p -T -X 32 -Y 48 -P 2 -i $<

res/spritehero8.pic res/spritehero8.pal: res/spritehero8.png
	$(GFX4SNES) -s 8 -o 16 -u 16 -p -T -X 16 -Y 16 -P 2 -i $<

# Extended clean
clean: clean-metasprite
.PHONY: clean-metasprite
clean-metasprite:
	@rm -f res/spritehero32.pic res/spritehero32.pal res/spritehero32.inc res/spritehero32_data.as res/spritehero32_meta.inc
	@rm -f res/spritehero16.pic res/spritehero16.pal res/spritehero16.inc res/spritehero16_data.as res/spritehero16_meta.inc
	@rm -f res/spritehero8.pic res/spritehero8.pal res/spritehero8.inc res/spritehero8_data.as res/spritehero8_meta.inc
