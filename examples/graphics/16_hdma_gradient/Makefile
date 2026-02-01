#==============================================================================
# HDMA Gradient Example
# Demonstrates HDMA for per-scanline color effects
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := hdma_gradient.sfc
ROM_NAME   := HDMA GRADIENT

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background hdma

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
