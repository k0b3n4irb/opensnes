#==============================================================================
# Smooth Movement Example - Makefile
#==============================================================================
# Demonstrates fixed-point math for sub-pixel movement

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := smooth_move.sfc
ROM_NAME := SMOOTH MOVEMENT

# Source files
CSRC     := main.c

# Enable OpenSNES library with math support
USE_LIB     := 1
LIB_MODULES := console input sprite dma math

include $(OPENSNES)/make/common.mk
