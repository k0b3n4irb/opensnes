# Memory Mapping - PVSnesLib example ported to OpenSNES
# Demonstrates HiROM-FastROM memory mapping
# -- alekmaul (original), DigiDwrf (feature)

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := memory_mapping.sfc
ROM_NAME := OPENSNES HIROM MEMMAP

USE_LIB  := 1
USE_HIROM := 1
LIB_MODULES := console dma text background sprite input

CSRC     := main.c

include $(OPENSNES)/make/common.mk
