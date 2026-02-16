#==============================================================================
# Mode 1 Background Example
# Simple tile mode 1 demo - displays static 16-color background
# Port of pvsneslib Mode1 example
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode1.sfc
ROM_NAME   := MODE1 BACKGROUND

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
