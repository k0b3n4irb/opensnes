#==============================================================================
# SNES Mouse Example Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mouse.sfc
ROM_NAME   := MOUSE DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console input sprite dma text background

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
