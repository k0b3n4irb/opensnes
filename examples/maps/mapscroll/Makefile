OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mapscroll.sfc
ROM_NAME   := MAP SCROLL DEMO

USE_LIB    := 1
LIB_MODULES := console sprite dma input background map

CSRC       := main.c
ASMSRC     := data.asm

TMX2SNES := $(OPENSNES)/bin/tmx2snes

include $(OPENSNES)/make/common.mk

# BG tileset (flags from PVSnesLib Makefile exactly)
res/tilesMario.pic res/tilesMario.pal res/tilesMario.map: res/tilesMario.png
	@echo "[GFX] $< -> tileset"
	@$(GFX4SNES) -s 8 -o 48 -u 16 -p -m -i $<

# Sprite (16x16, 4 frames)
res/mario.pic res/mario.pal: res/mario.png
	@echo "[GFX] $< -> sprite"
	@$(GFX4SNES) -s 16 -o 16 -u 16 -p -i $<

# Tiled map conversion
res/BG1.m16 res/tiledMario.t16 res/tiledMario.b16: res/tiledMario.tmj res/tilesMario.map
	@echo "[TMX] $< -> map binaries"
	@cd res && $(TMX2SNES) tiledMario.tmj tilesMario.map

combined.asm: res/tilesMario.pic res/mario.pic res/BG1.m16
