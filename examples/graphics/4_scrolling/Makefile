#==============================================================================
# Scrolling Example Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := scrolling.sfc
ROM_NAME   := SCROLLING DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console input sprite

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
