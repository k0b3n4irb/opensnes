OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := hicolor_hires.sfc
ROM_NAME := HICOLOR HIRES

USE_LIB  := 1
LIB_MODULES := console dma background colormath

CSRC   := main.c
ASMSRC := data.asm irq_stream.asm

include $(OPENSNES)/make/common.mk
