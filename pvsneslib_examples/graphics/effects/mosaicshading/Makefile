OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mosaicshading.sfc
ROM_NAME := OPENSNES MOSAICSHADE

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mosaic

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
