# Mode 7 - PVSnesLib example ported to OpenSNES
# Demonstrates Mode 7 rotation and scaling with joypad input

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode7.sfc
ROM_NAME := OPENSNES MODE7

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mode7

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
