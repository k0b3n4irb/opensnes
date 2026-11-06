# LZ77 into RAM — lzssDecodeRam, screen on, banks $7E and $7F

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := lzss_ram.sfc
ROM_NAME := LZSS RAM

USE_LIB     := 1
LIB_MODULES := console dma text background sprite lzss

CSRC   := main.c
ASMSRC := streams.asm

include $(OPENSNES)/make/common.mk
