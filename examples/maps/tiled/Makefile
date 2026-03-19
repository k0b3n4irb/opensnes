OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := tiled.sfc
ROM_NAME   := TILED MAP DEMO

USE_LIB    := 1
LIB_MODULES := console sprite dma input background map

CSRC       := main.c
ASMSRC     := data.asm

TMX2SNES := $(OPENSNES)/bin/tmx2snes

include $(OPENSNES)/make/common.mk

# Convert tileset PNG to SNES tiles + palette + tileset map
# Flags match PVSnesLib exactly: -s 8 -o 48 -u 16 -p -m
res/tileslevel1.pic res/tileslevel1.pal res/tileslevel1.map: res/tileslevel1.png
	@echo "[GFX] $< -> tileset"
	@$(GFX4SNES) -s 8 -o 48 -u 16 -p -m -i $<

# Convert Tiled JSON map to SNES binary format
# Needs the tileset .map for tile index optimization
res/BG1.m16 res/maplevel01.t16 res/maplevel01.b16: res/maplevel01.tmj res/tileslevel1.map
	@echo "[TMX] $< -> map binaries"
	@cd res && $(TMX2SNES) maplevel01.tmj tileslevel1.map

combined.asm: res/tileslevel1.pic res/BG1.m16
