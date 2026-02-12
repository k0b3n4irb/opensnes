# DynamicSprite - PVSnesLib example ported to OpenSNES
# Demonstrates animated sprites using the dynamic sprite engine

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := dynamicsprite.sfc
ROM_NAME := OPENSNES DYNSPRITE

USE_LIB     := 1
LIB_MODULES := console dma background sprite sprite_dynamic sprite_lut input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
