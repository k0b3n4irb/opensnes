OPENSNES := $(shell cd ../../../.. && pwd)

TARGET      := superfx_bitmap.sfc
ROM_NAME    := SUPERFX BITMAP

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_render.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background text superfx

include $(OPENSNES)/make/common.mk
