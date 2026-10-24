#==============================================================================
# rooftop_run - Kit's Rooftop Run, a side-scrolling platformer
#==============================================================================
# Demonstrates: tile-based scrolling, sprite animation, physics, collision,
#               camera tracking, and SNESMOD music playback.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := rooftop_run.sfc
ROM_NAME    := ROOFTOP RUN

CSRC        := main.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background anim vramqueue text

USE_SNESMOD    := 1
SOUNDBANK_SRC  := res/rooftops.it res/jump_sfx.it

include $(OPENSNES)/make/common.mk
