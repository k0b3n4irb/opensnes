#==============================================================================
# SNESMOD Music Example
#==============================================================================
#
# This example demonstrates music playback using SNESMOD.
#
# To build this example:
#   1. Place your Impulse Tracker (.it) file in music/
#   2. Update SOUNDBANK_SRC below with the filename
#   3. Run 'make'
#
# Creating music:
#   - Use OpenMPT (free, https://openmpt.org/) to create .it files
#   - SNES supports 8 channels, 32kHz sample rate
#   - Keep total sample data under 58KB
#   - Export as Impulse Tracker format (.it)
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := music.sfc
ROM_NAME := OPENSNES SNESMOD MUS

CSRC     := main.c

# Enable SNESMOD audio system
USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input

# Soundbank source files
SOUNDBANK_SRC  := music/pollen8.it

include $(OPENSNES)/make/common.mk
