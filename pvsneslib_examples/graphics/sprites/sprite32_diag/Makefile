# sprite32_diag - Diagnostic test for 32x32 sprite rendering
# Tests direct buffer writes vs library functions

OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := sprite32_diag.sfc
ROM_NAME := OPENSNES SPR32 DIAG

USE_LIB  := 1
LIB_MODULES := console dma sprite input background text

CSRC     := main.c
ASMSRC   := data.asm

include $(OPENSNES)/make/common.mk
