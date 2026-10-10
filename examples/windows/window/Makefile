#==============================================================================
# Window Masking Example (HDMA Triangle)
# Ported from PVSnesLib — triangle window via HDMA, toggled with A/X/B
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := window.sfc
ROM_NAME   := WINDOW HDMA DEMO

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background window hdma

# Source files
CSRC       := main.c

# Include common build rules
include $(OPENSNES)/make/common.mk
