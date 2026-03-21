#==============================================================================
# SNESMOD Large Music Example (>32KB soundbank)
#==============================================================================
#
# Demonstrates music playback with a soundbank larger than 32KB.
# In LoROM, soundbanks >32KB are automatically split across multiple
# ROM banks by smconv. The SNESMOD library handles bank crossing
# transparently via the incptr macro.
#
# "What Is Love" by Haddaway — 108KB IT module → 56KB soundbank (2 banks)
#
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := music_large.sfc
ROM_NAME := SNESMOD_LARGEMUS

CSRC     := main.c

USE_SNESMOD    := 1
USE_LIB        := 1
LIB_MODULES    := console sprite dma input background text

SOUNDBANK_SRC  := music/whatislove.it

include $(OPENSNES)/make/common.mk
