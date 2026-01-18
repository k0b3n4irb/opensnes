#==============================================================================
# SNESMOD Sound Effects Example
#==============================================================================
#
# This example demonstrates sound effect playback using SNESMOD.
# Unlike music, effects are loaded individually and can be played
# with various pitch and volume settings.
#
# The sfx/effectssfx.it file contains 5 sample instruments:
#   0: Tada
#   1: Hall Strings
#   2: Honky Tonk Piano
#   3: Marimba
#   4: Cowbell
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sfx.sfc
ROM_NAME := OPENSNES SNESMOD SFX

CSRC     := main.c

# Enable SNESMOD audio system
USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite input

# Soundbank source files (effects only, no music module)
SOUNDBANK_SRC  := sfx/effectssfx.it

include $(OPENSNES)/make/common.mk
