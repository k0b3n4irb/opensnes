OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := map_scroll.sfc
ROM_NAME   := MAP SCROLL DEMO

USE_LIB    := 1
LIB_MODULES := console sprite dma input background map

CSRC       := main.c
ASMSRC     := data.asm

TMX2SNES := $(OPENSNES)/bin/tmx2snes

include $(OPENSNES)/make/common.mk

# BG tileset (flags from PVSnesLib Makefile exactly)

# Sprite (16x16, 4 frames)

# Tiled map conversion
# the .map comes from res/tilesMario.png.toml (opensnes-tileset); tmx2snes (not yet in the family) reads it
res/tilesMario.map: | res/tilesMario.png.toml.done
res/BG1.m16 res/tiledMario.t16 res/tiledMario.b16: res/tiledMario.tmj res/tilesMario.map
	@echo "[TMX] $< -> map binaries"
	@cd res && $(TMX2SNES) tiledMario.tmj tilesMario.map
