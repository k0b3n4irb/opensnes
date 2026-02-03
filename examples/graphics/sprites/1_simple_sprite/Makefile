#==============================================================================
# Simple Sprite Example
# Displays a static 32x32 sprite
# Port of pvsneslib SimpleSprite example
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := simple_sprite.sfc
ROM_NAME   := SIMPLE SPRITE

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
