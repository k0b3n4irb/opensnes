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
# Graphics conversion (4bpp Mode 1 background — composed grass-biome scene)
#------------------------------------------------------------------------------
# scene.png is 256×256 (exactly one SNES screen) with 12 unique colours,
# built by res/compose_scene.py from the 16×16 source tiles in ground.png.
# A pure grass-fill (6,2) covers the whole screen with trees, two red tent
# enemy bases and decorative bushes/rings sprinkled on top.

res/scene.pic res/scene.pal res/scene.map: res/scene.png
	@echo "[GFX] Converting scene tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

combined.asm: res/scene.pic

clean: clean-shmup_1942
.PHONY: clean-shmup_1942
clean-shmup_1942:
	@rm -f res/scene.pic res/scene.pal res/scene.map res/scene.inc res/scene_data.as
