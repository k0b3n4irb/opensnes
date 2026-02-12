#==============================================================================
# BRR Sample Playback Demo (tada)
#==============================================================================
#
# Port of PVSnesLib "tada" example.
# Demonstrates loading and playing a BRR sound sample using the
# OpenSNES audio system (not SNESMOD).
#
# Press A to play the tada sound effect.
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := tada.sfc
ROM_NAME := OPENSNES TADA SFX

USE_LIB     := 1
LIB_MODULES := console dma sprite input text audio background

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
