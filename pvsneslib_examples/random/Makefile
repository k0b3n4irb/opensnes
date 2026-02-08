# Random Number Demo - PVSnesLib example ported to OpenSNES
# Tests text module + input + rand()

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := random.sfc
ROM_NAME := OPENSNES RANDOM  NUM

USE_LIB  := 1
LIB_MODULES := console dma text input background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
