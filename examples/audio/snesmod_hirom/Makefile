#==============================================================================
# SNESMOD HiROM Music Example
#==============================================================================
#
# Same as 3_snesmod_music but built in HiROM mode (64KB banks).
# Demonstrates that SNESMOD audio works correctly in HiROM.
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := hirom_music.sfc
ROM_NAME := OPENSNES HIROM MUSIC

CSRC     := main.c

# Enable SNESMOD audio system + HiROM mode
USE_SNESMOD    := 1
USE_HIROM      := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input

# Soundbank source files (shared with 3_snesmod_music)
SOUNDBANK_SRC  := music/pollen8.it

include $(OPENSNES)/make/common.mk
