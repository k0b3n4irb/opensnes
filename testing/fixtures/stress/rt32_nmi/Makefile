OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := rt32_nmi.sfc
ROM_NAME := RT32 NMI PROBE
USE_LIB  := 1
LIB_MODULES := console

CSRC   := main.c
ASMSRC := irq.asm    # the H-timer IRQ handler (irqSet takes ASM handlers only)

include $(OPENSNES)/make/common.mk
