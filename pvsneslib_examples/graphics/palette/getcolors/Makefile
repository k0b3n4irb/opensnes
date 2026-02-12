# GetColors - PVSnesLib example ported to OpenSNES
# Demonstrates reading palette colors from CGRAM

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := getcolors.sfc
ROM_NAME := OPENSNES GETCOLORS

USE_LIB  := 1
LIB_MODULES := console dma sprite

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
