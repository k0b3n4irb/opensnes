# HDMA Gradient - PVSnesLib example ported to OpenSNES
# Demonstrates Mode 3 (256 colors) with >32KB tiles and HDMA brightness gradient
# Press A to cycle through gradient levels

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := hdmagradient.sfc
ROM_NAME := OPENSNES HDMAGRAD

USE_LIB  := 1
LIB_MODULES := console dma background sprite input hdma math

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
