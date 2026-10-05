#==============================================================================
# Transparent Window Example
# Ported from PVSnesLib — semi-transparent rectangle using color + HDMA
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := transparent_window.sfc
ROM_NAME   := TRANSPARENT WINDOW

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma background window colormath hdma

# Source files
CSRC       := main.c

# Include common build rules
include $(OPENSNES)/make/common.mk
