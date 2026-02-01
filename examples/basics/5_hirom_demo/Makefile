#==============================================================================
# HiROM Demo - Makefile
#==============================================================================
# Demonstrates HiROM mode (64KB banks) vs default LoROM (32KB banks).
# The only difference from a normal Makefile is USE_HIROM := 1

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := hirom_demo.sfc
ROM_NAME := HIROM DEMO

# Source files
CSRC     := main.c

# HiROM + Library support (Phase 2)
USE_LIB     := 1
LIB_MODULES := console dma input sprite

# Enable HiROM mode (64KB banks instead of 32KB)
USE_HIROM   := 1

include $(OPENSNES)/make/common.mk
