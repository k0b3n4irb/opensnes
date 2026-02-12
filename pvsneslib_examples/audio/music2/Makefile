#==============================================================================
# SNESMOD Music with Pause/Resume Demo (music2)
#==============================================================================
#
# Port of PVSnesLib "music2" example.
# Demonstrates music playback with pause and resume controls.
#
# Controls:
#   A - Pause music
#   B - Resume music
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := music2.sfc
ROM_NAME := OPENSNES MUSIC2 DEMO

USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input text background

CSRC   := main.c

SOUNDBANK_SRC  := res/pollen8.it

include $(OPENSNES)/make/common.mk
