# SRAM Offset - PVSnesLib example ported to OpenSNES
# Multi-slot SRAM save/load with offset support

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sramoffset.sfc
ROM_NAME := OPENSNES SRAM OFFSET

USE_LIB  := 1
LIB_MODULES := console dma text background sprite input sram

CSRC     := main.c

include $(OPENSNES)/make/common.mk
