# SA-1 library fixture (2026-09-26): the sram module on an SA-1 cartridge.
# The save memory is BW-RAM at $40:0000, writable by the SNES CPU once crt0
# sets SBWE ($2226); until then USE_SRAM=1 with USE_SA1=1 was refused.
OPENSNES := $(shell cd ../.. && pwd)
TARGET   := libtest_sa1_sram.sfc
ROM_NAME := LIB RUNTIME SA1 SRAM
USE_LIB  := 1
USE_SA1  := 1
USE_SRAM := 1
LIB_MODULES := console dma
CSRC   := main.c
include $(OPENSNES)/make/common.mk
