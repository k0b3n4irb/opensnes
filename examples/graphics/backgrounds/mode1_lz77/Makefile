# Mode 1 LZ77 - Compressed tile demo using LZSS decompression (from PVSnesLib)

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode1_lz77.sfc
ROM_NAME := MODE1 LZ77 DEMO

USE_LIB     := 1
LIB_MODULES := console lzss background sprite

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
