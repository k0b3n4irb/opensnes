#==============================================================================
# Fading Effect Example
# Screen fade in/out demonstration
# Port of pvsneslib Fading example
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := fading.sfc
ROM_NAME   := FADING EFFECT

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
