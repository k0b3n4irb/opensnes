# Object Size Example
# Interactive demo of all 6 SNES sprite size modes

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := sprite_sizes.sfc
ROM_NAME   := OBJECT SIZE

USE_LIB    := 1
LIB_MODULES := console sprite dma text text4bpp input background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp sprites at 4 sizes)
#------------------------------------------------------------------------------

res/sprite8.pic res/sprite8.pal: res/sprite8.png
	@echo "[GFX] Converting 8x8 sprite..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -i $<

res/sprite16.pic res/sprite16.pal: res/sprite16.png
	@echo "[GFX] Converting 16x16 sprite..."
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

res/sprite32.pic res/sprite32.pal: res/sprite32.png
	@echo "[GFX] Converting 32x32 sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

res/sprite64.pic res/sprite64.pal: res/sprite64.png
	@echo "[GFX] Converting 64x64 sprite..."
	@$(GFX4SNES) -s 64 -o 16 -u 16 -p -i $<

combined.asm: res/sprite8.pic res/sprite16.pic res/sprite32.pic res/sprite64.pic

clean: clean-sprite_sizes
.PHONY: clean-sprite_sizes
clean-sprite_sizes:
	@rm -f res/sprite8.pic res/sprite8.pal res/sprite8.inc res/sprite8_data.as
	@rm -f res/sprite16.pic res/sprite16.pal res/sprite16.inc res/sprite16_data.as
	@rm -f res/sprite32.pic res/sprite32.pal res/sprite32.inc res/sprite32_data.as
	@rm -f res/sprite64.pic res/sprite64.pal res/sprite64.inc res/sprite64_data.as
