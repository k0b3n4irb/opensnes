# Mode 7 Perspective - F-Zero style ground perspective with HDMA sky
# Based on PVSnesLib Mode7Perspective by mills32 / alekmaul

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := mode7_perspective.sfc
ROM_NAME := OPENSNES MODE7 PERSP

USE_LIB     := 1
LIB_MODULES := console dma background sprite input mode7

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
