OPENSNES := $(shell cd ../.. && pwd)

TARGET   := libtest.sfc
ROM_NAME := LIB RUNTIME TEST

USE_LIB  := 1
# L2c (2026-09-15): collision, window and sram get their runtime asserts
# here; USE_SRAM maps bank $70 in the header (common.mk adds the module).
USE_SRAM := 1
LIB_MODULES := console sprite dma background text math anim map audio fixed32 collision window input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
