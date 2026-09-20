# HiROM library fixture (2026-09-20): the sram module on a HiROM cartridge.
# HiROM maps battery RAM at $30-$3F:$6000-$7FFF, not at LoROM's $70:0000; the
# module used the LoROM values for every build until this fixture existed.
OPENSNES := $(shell cd ../.. && pwd)
TARGET   := libtest_hirom.sfc
ROM_NAME := LIB RUNTIME HIROM
USE_LIB   := 1
USE_HIROM := 1
USE_SRAM  := 1
LIB_MODULES := console dma
CSRC   := main.c
include $(OPENSNES)/make/common.mk
