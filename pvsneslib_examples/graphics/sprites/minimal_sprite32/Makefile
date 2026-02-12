# Minimal 32x32 Sprite Test - OpenSNES regression test
# Verifies that 32x32 sprites display correctly

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := minimal_sprite32.sfc
ROM_NAME := OPENSNES SPRITE32TST

USE_LIB  := 1
LIB_MODULES := console dma sprite

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
