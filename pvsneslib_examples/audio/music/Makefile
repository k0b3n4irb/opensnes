#==============================================================================
# SNESMOD Music Playback Demo (music)
#==============================================================================
#
# Port of PVSnesLib "music" example.
# Demonstrates simple music playback with auto-loop using SNESMOD.
# Background color cycles while music plays.
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := music.sfc
ROM_NAME := OPENSNES MUSIC DEMO

USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input text background

CSRC   := main.c

SOUNDBANK_SRC  := res/pollen8.it

include $(OPENSNES)/make/common.mk
