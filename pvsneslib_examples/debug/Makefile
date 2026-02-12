# Debug - PVSnesLib example ported to OpenSNES
# Debug output demo (on-screen, replacing no$sns debug messages)

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := debug.sfc
ROM_NAME := OPENSNES DEBUG

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
