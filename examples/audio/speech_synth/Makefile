OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := speech_synth.sfc
ROM_NAME := SPEECH SYNTH

USE_LIB  := 1
LIB_MODULES := console apu

CSRC   := main.c
ASMSRC := data.asm
SPCSRC := player.spc700.asm

include $(OPENSNES)/make/common.mk
