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

# NOTE: Sprite .pic/.pal files are pre-converted from PVSnesLib because
# gfx4snes cannot force 8bpp encoding (PVSnesLib uses gfx2snes -u 256).
# The BMP source files are kept in res/ for reference.
