# ObjectSize - PVSnesLib example ported to OpenSNES
# Demonstrates all 6 OBJ size combinations

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := objectsize.sfc
ROM_NAME := OPENSNES OBJECTSIZE

USE_LIB  := 1
LIB_MODULES := console dma text background sprite input

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
