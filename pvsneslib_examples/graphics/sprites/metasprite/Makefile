# MetaSprite - PVSnesLib example ported to OpenSNES
# Demonstrates multi-size metasprites (32x32, 16x16, 8x8) over a background

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := metasprite.sfc
ROM_NAME := OPENSNES METASPRITE

USE_LIB     := 1
LIB_MODULES := console dma background sprite input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
