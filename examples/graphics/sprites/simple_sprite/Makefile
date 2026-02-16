# Simple Sprite - Displays a single 32x32 sprite at center screen

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := simple_sprite.sfc
ROM_NAME := OPENSNES SIMPLESPRT

USE_LIB     := 1
LIB_MODULES := console dma sprite

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
