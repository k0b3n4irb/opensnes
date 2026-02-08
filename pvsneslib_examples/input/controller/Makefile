# Controller Input Demo - PVSnesLib example ported to OpenSNES
# Tests padHeld() with text display for all buttons

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := controller.sfc
ROM_NAME := OPENSNES  CONTROLLER

USE_LIB  := 1
LIB_MODULES := console dma text input background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
