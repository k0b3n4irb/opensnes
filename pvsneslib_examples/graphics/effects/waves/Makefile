# Waves Effect - PVSnesLib example ported to OpenSNES
# Demonstrates HDMA-driven horizontal wave distortion on BG1
# A = stop waves, B = restart waves

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := waves.sfc
ROM_NAME := OPENSNES WAVES

USE_LIB  := 1
LIB_MODULES := console dma background sprite input hdma math

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
