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

CSRC       := main.c map32x32.c map64x64.c
ASMSRC     := ram.asm

include $(OPENSNES)/make/common.mk
