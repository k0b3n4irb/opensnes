# Mode 1 BG3 High Priority Example
# Demonstrates BG3 as a HUD layer overlaying BG1+BG2 in Mode 1

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode1_bg3_priority.sfc
ROM_NAME   := MODE1 BG3 PRIORITY

USE_LIB    := 1
LIB_MODULES := console sprite dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk
