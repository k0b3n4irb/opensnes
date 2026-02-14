# Arithmetic - PVSnesLib example ported to OpenSNES
# Integer calculator with button grid, d-pad cursor, +/-/*/÷/^ operations

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := arithmetic.sfc
ROM_NAME := OPENSNES ARITHMETIC

USE_LIB  := 1
LIB_MODULES := console sprite dma input

CSRC     := main.c

include $(OPENSNES)/make/common.mk
