#==============================================================================
# Color Math Transparency Example
# Demonstrates shadow and tint effects using color math
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := transparency.sfc
ROM_NAME   := COLOR MATH DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background colormath

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
