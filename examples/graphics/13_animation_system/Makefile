#==============================================================================
# Animation System Example - Makefile
#==============================================================================
# Demonstrates the sprite animation framework

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := animation_demo.sfc
ROM_NAME := ANIMATION DEMO

# Source files
CSRC     := main.c

# Enable OpenSNES library with animation support
USE_LIB     := 1
LIB_MODULES := console input sprite dma animation

include $(OPENSNES)/make/common.mk
