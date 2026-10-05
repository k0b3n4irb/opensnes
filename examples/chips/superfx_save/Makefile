# Super FX save: a boot counter kept in battery-backed Game Pak RAM across
# power cycles. USE_SUPERFX + USE_SRAM: the header says $15 (GSU + battery),
# the RAM size at $FFBD (GSU_RAM_KB, 64), and the sram module addresses
# $70:0000 — the GSU's RAM, which is also the cartridge's save memory.
OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := superfx_save.sfc
ROM_NAME    := SUPERFX SAVE

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_idle.sfx

USE_LIB     := 1
USE_SUPERFX := 1
USE_SRAM    := 1
LIB_MODULES := console dma background text sram   # superfx auto-added by USE_SUPERFX

include $(OPENSNES)/make/common.mk
