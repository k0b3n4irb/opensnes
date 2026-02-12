# Timer - PVSnesLib example ported to OpenSNES
# Displays a VBlank frame counter that increments each frame

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := timer.sfc
ROM_NAME := OPENSNES TIMER

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
