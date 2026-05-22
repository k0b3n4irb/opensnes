#==============================================================================
# shmup_1942 — S1: Static tile library preview
#
# Vertical 1942-style shoot 'em up, built on Kenney's CC0 Pixel Shmup pack.
# S1 just proves the gfx4snes → bgLoad pipeline by displaying the
# ground tile library on BG1 with no scrolling and no sprites.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := shmup_1942.sfc
ROM_NAME   := SHMUP 1942

USE_LIB    := 1
LIB_MODULES := console dma background asset

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp Mode 1 background — grass biome terrain)
#------------------------------------------------------------------------------
# ground.png is 192×112 with 18 unique colors. gfx4snes quantises to 16 (2
# colours folded into nearest neighbours) and produces a 24×14 tilemap. The
# tilemap occupies the top-left of the 32×32 SNES screen at scroll (0,0); the
# rest of the screen falls back to tile 0 from the converted library.

res/ground.pic res/ground.pal res/ground.map: res/ground.png
	@echo "[GFX] Converting ground tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

combined.asm: res/ground.pic

clean: clean-shmup_1942
.PHONY: clean-shmup_1942
clean-shmup_1942:
	@rm -f res/ground.pic res/ground.pal res/ground.map res/ground.inc res/ground_data.as
