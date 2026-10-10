# SA-1 save: a boot counter kept in battery-backed BW-RAM across power cycles.
# USE_SA1 + USE_SRAM: the header says $35 (SA-1 + battery), crt0 enables
# SNES-side writes to BW-RAM (SBWE), and the sram module addresses $40:0000.
OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sa1_save.sfc
ROM_NAME := SA1 SAVE

USE_LIB  := 1
USE_SA1  := 1
USE_SRAM := 1
LIB_MODULES := console dma background text sa1 sram

CSRC     := main.c

include $(OPENSNES)/make/common.mk
