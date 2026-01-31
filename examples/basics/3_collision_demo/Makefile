#==============================================================================
# Collision Demo Example - Makefile
#==============================================================================
# Demonstrates AABB and tile-based collision detection

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := collision_demo.sfc
ROM_NAME := COLLISION DEMO

# Source files
CSRC     := main.c

# Enable OpenSNES library with collision support
USE_LIB     := 1
LIB_MODULES := console input sprite dma collision

include $(OPENSNES)/make/common.mk
