# SRAM Simple - PVSnesLib example ported to OpenSNES
# Basic SRAM save/load: save 0xCAFE to SRAM, load it back

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sramsimple.sfc
ROM_NAME := OPENSNES SRAM SIMPLE

USE_LIB  := 1
LIB_MODULES := console dma text background sprite input sram

CSRC     := main.c

include $(OPENSNES)/make/common.mk
