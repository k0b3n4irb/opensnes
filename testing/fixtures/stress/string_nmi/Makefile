OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := string_nmi.sfc
ROM_NAME := STRING NMI PROBE
USE_LIB  := 1
LIB_MODULES := console string

CSRC   := main.c
ASMSRC := data.asm    # the ROM source table of the callback

include $(OPENSNES)/make/common.mk
