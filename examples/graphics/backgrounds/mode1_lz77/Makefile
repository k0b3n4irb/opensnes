# Mode 1 LZ77 - Compressed tile demo using LZSS decompression (from PVSnesLib)

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode1_lz77.sfc
ROM_NAME := MODE1 LZ77 DEMO

USE_LIB     := 1
LIB_MODULES := console lzss background sprite

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion (4bpp Mode 1, LZ77 compressed)
#------------------------------------------------------------------------------

res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map: res/pvsneslib.png
	@echo "[GFX] Converting background (LZ77)..."
	@$(GFX4SNES) -z -s 8 -o 16 -u 16 -e 0 -p -m -i $<

combined.asm: res/pvsneslib.pic

clean: clean-mode1_lz77
.PHONY: clean-mode1_lz77
clean-mode1_lz77:
	@rm -f res/pvsneslib.pic res/pvsneslib.pal res/pvsneslib.map res/pvsneslib.inc res/pvsneslib_data.as
