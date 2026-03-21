#==============================================================================
# SNESMOD HiROM Music Example
#==============================================================================
#
# Demonstrates music playback using HiROM (64KB banks).
# HiROM allows larger soundbanks since each bank is 64KB instead of 32KB.
#
# "What Is Love" extended — 210KB IT module.
#
# Key differences from LoROM:
#   - USE_HIROM=1 enables 64KB bank layout
#   - smconv receives -i flag for HiROM address mapping
#   - Soundbank ORG is $8000 (not 0) for bank mirror access
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := music_hirom.sfc
ROM_NAME := SNESMOD_HIROM_MUS

CSRC     := main.c

USE_SNESMOD    := 1
USE_HIROM      := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input background text

SOUNDBANK_SRC  := music/whatislove.it

include $(OPENSNES)/make/common.mk
