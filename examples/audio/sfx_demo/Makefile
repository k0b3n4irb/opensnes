#==============================================================================
# SFX Demo - Play a BRR sample via bare-metal SPC700 driver
#==============================================================================
#
# Demonstrates uploading a custom SPC700 driver and BRR sample,
# then triggering playback from C code. Uses the OpenSNES library
# for initialization and input handling.
#
# Press A to play the TADA sound effect.
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sfx_demo.sfc
ROM_NAME := OPENSNES SFX DEMO

USE_LIB     := 1
LIB_MODULES := console input sprite dma

CSRC     := main.c
ASMSRC   := spc.asm

include $(OPENSNES)/make/common.mk
