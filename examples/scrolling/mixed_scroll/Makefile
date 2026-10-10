#==============================================================================
# Mode 1 Mixed Scroll Example
# Two Mode 1 backgrounds: BG1 (shader) auto-scrolls, BG2 (opensnes logo) fixed
# Port of PVSnesLib Mode1MixedScroll example
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mixed_scroll.sfc
ROM_NAME   := MODE1 MIXED SCROLL

USE_LIB    := 1
LIB_MODULES := console sprite dma background

CSRC       := main.c

include $(OPENSNES)/make/common.mk
