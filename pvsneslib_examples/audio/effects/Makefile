#==============================================================================
# SNESMOD Sound Effects Demo (effects)
#==============================================================================
#
# Port of PVSnesLib "effects" example.
# Demonstrates loading and playing 5 sound effects from an
# Impulse Tracker file using SNESMOD.
#
# The res/effectssfx.it file contains 5 sample instruments:
#   0: Tada
#   1: Hall Strings
#   2: Honky Tonk Piano
#   3: Marimba 1
#   4: Cowbell
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := effects.sfc
ROM_NAME := OPENSNES SFX DEMO

USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input text background

CSRC   := main.c

SOUNDBANK_SRC  := res/effectssfx.it

include $(OPENSNES)/make/common.mk
