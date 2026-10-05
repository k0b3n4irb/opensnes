#==============================================================================
# LikeMario - Side-scrolling platformer example
#==============================================================================
# Demonstrates: tile-based scrolling, sprite animation, physics, collision,
#               camera tracking, and SNESMOD music playback.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := likemario.sfc
ROM_NAME    := OPENSNES LIKEMARIO

CSRC        := main.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background anim

USE_SNESMOD    := 1
SOUNDBANK_SRC  := res/overworld.it res/jump_sfx.it

include $(OPENSNES)/make/common.mk
