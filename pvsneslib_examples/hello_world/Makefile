# Hello World - PVSnesLib example ported to OpenSNES
# First real test of the text module

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := hello_world.sfc
ROM_NAME := OPENSNES HELLO WORLD

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
