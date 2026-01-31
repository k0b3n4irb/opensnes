# Dynamic Sprite Engine Example Makefile

OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := dynamic_sprite.sfc
ROM_NAME := DYNAMIC_SPRITE_DEMO

CSRC     := main.c
ASMSRC   := data.asm

USE_LIB  := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma background input

include $(OPENSNES)/make/common.mk
