#==============================================================================
# Window Masking Example
# Demonstrates window masking for spotlight effect
#==============================================================================

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := window.sfc
ROM_NAME   := WINDOW DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background window

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
