# GradientColors - PVSnesLib example ported to OpenSNES
# Demonstrates HDMA color gradient effect

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := gradientcolors.sfc
ROM_NAME := OPENSNES GRADIENT

USE_LIB  := 1
LIB_MODULES := console dma background sprite hdma input math

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
