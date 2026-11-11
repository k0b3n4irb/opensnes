OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := string_block.sfc
ROM_NAME := STRING BLOCK PROBE
USE_LIB  := 1
LIB_MODULES := console string

CSRC   := main.c
ASMSRC := data.asm    # the ROM source table, sb_dbr, the NMI switch, sb_mark

include $(OPENSNES)/make/common.mk
