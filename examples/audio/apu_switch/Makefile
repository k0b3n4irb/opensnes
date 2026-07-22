OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := apu_switch.sfc
ROM_NAME := APU SWITCH
USE_LIB  := 1
LIB_MODULES := console apu input

CSRC   := main.c
ASMSRC := data.asm
SPCSRC := drums.spc700.asm cello.spc700.asm

include $(OPENSNES)/make/common.mk
