# Window Effect - PVSnesLib example ported to OpenSNES
# Demonstrates HDMA-driven window masking on BG1/BG2
# A = BG1 only, X = BG2 only, B = both

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := window.sfc
ROM_NAME := OPENSNES WINDOW

USE_LIB  := 1
LIB_MODULES := console dma background sprite input hdma math

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
