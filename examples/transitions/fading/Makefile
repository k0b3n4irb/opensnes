#==============================================================================
# Screen transitions: brightness fades and the mosaic filter
# Port of PVSnesLib's Fading and Mosaic examples, in one ROM
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := fading.sfc
ROM_NAME   := FADE AND MOSAIC

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console sprite dma input background mosaic

# Source files
CSRC       := main.c

# Include common build rules
include $(OPENSNES)/make/common.mk
