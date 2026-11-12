OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := world_place.sfc
ROM_NAME := WORLD PLACE PROBE
USE_LIB  := 1
LIB_MODULES := console sprite

CSRC   := main.c
ASMSRC := data.asm    # the ROM arrays, the NMI switch, the data bank

include $(OPENSNES)/make/common.mk
