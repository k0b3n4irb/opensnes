# Breakpoints - PVSnesLib example ported to OpenSNES
# Demonstrates WDM breakpoint for Mesen debugger

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := breakpoints.sfc
ROM_NAME := OPENSNES BREAKPOINTS

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c
ASMSRC   := wdm.asm

include $(OPENSNES)/make/common.mk
