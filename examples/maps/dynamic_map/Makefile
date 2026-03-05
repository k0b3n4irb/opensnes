#==============================================================================
# DynamicMap Example
# Custom tilemap-based sprite engine with 32x32 and 64x64 map modes.
# Mode 3 (256 colors BG1, 16 colors BG2 for text).
# Port of PVSnesLib DynamicMap example.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := dynamic_map.sfc
ROM_NAME   := DYNAMIC MAP

USE_LIB    := 1
LIB_MODULES := console sprite dma background text text4bpp input

CSRC       := main.c map32x32.c map64x64.c maputil.c
ASMSRC     := data.asm ram.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (8bpp sprites for Mode 3 BG1)
#------------------------------------------------------------------------------

# 32x32 map mode: 8x8 tiles, 8bpp, with tilemap
res/sprite16.pic res/sprite16.pal res/sprite16.map: res/sprite16.png
	@echo "[GFX] Converting 8bpp sprite (32x32 map)..."
	@$(GFX4SNES) -s 8 -o 256 -u 256 -p -m -i $<

# 64x64 map mode: 16x16 tiles, 8bpp, NO tilemap (code relies on tile gap layout)
res/sprite16_64x64.pic res/sprite16_64x64.pal: res/sprite16_64x64.png
	@echo "[GFX] Converting 8bpp sprite (64x64 map)..."
	@$(GFX4SNES) -s 16 -o 256 -u 256 -p -i $<

combined.asm: res/sprite16.pic res/sprite16_64x64.pic

clean: clean-dynamic_map
.PHONY: clean-dynamic_map
clean-dynamic_map:
	@rm -f res/sprite16.pic res/sprite16.pal res/sprite16.map res/sprite16.inc res/sprite16_data.as
	@rm -f res/sprite16_64x64.pic res/sprite16_64x64.pal res/sprite16_64x64.inc res/sprite16_64x64_data.as
