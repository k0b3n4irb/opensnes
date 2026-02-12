# Test Region - PVSnesLib example ported to OpenSNES
# Compares SNES hardware region with cartridge region byte
# Note: hdr.asm sets COUNTRY $02 (PAL) to match PAL hardware

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := testregion.sfc

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
