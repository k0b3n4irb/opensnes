# DSP-1 ground — Super Mario Kart style Mode 7 floor: the per-scanline
# matrices come from the DSP-1 Raster command instead of a precomputed table.
# Assets reused from mode7/perspective (PVSnesLib Mode7Perspective, mills32 / alekmaul).

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := dsp1_ground.sfc
ROM_NAME := DSP1 GROUND

USE_LIB     := 1
USE_DSP1    := 1
LIB_MODULES := console dma background input mode7 hdma   # dsp1 auto-added by USE_DSP1

CSRC   := main.c
include $(OPENSNES)/make/common.mk
