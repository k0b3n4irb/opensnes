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
# Requirements:
#   - smconv tool (requires Go to build)
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

# Check if smconv is available
SMCONV := $(OPENSNES)/bin/smconv
SMCONV_EXISTS := $(shell test -x $(SMCONV) && echo yes)

ifeq ($(SMCONV_EXISTS),yes)

TARGET   := music.sfc
ROM_NAME := OPENSNES SNESMOD MUS

CSRC     := main.c

# Enable SNESMOD audio system
USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite input

# Soundbank source files
SOUNDBANK_SRC  := music/pollen8.it

include $(OPENSNES)/make/common.mk

else

# smconv not available - skip this example
.PHONY: all clean

all:
	@echo "[SKIP] 6_snesmod_music - smconv not installed (requires Go)"

clean:
	@rm -f *.sfc *.sym combined.* linkfile *.o *.obj soundbank.* *.asm

endif
