# Mode1BG3HighPriority - PVSnesLib example ported to OpenSNES
# Demonstrates 3 BG layers in Mode 1 with BG3 as high-priority HUD overlay

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode1bg3hp.sfc
ROM_NAME := OPENSNES MODE1BG3HP

USE_LIB     := 1
LIB_MODULES := console dma background sprite

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
