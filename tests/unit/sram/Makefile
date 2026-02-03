# SRAM Unit Test Makefile

OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := test_sram.sfc
ROM_NAME := SRAM TESTS

CSRC     := test_sram.c
USE_LIB  := 1
USE_SRAM := 1
LIB_MODULES := console dma text sprite

include $(OPENSNES)/make/common.mk
