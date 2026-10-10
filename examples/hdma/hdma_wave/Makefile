# HDMA Wave: build the table by hand, then let the hdma module do it

OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := hdma_wave.sfc
ROM_NAME := HDMA WAVE DEMO

CSRC     := main.c
ASMSRC   := data.asm
USE_LIB  := 1
LIB_MODULES := console dma background input hdma

include $(OPENSNES)/make/common.mk

# 8bpp 256-color conversion (same pipeline as backgrounds/mode3)
