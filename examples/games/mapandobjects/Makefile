#==============================================================================
# Map and Objects - Scrolling platformer with object engine
#==============================================================================
# Demonstrates: map engine scrolling, object engine with physics/collision,
#               dynamic sprites, enemy AI (goomba, koopa troopa).
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := mapandobjects.sfc
ROM_NAME    := OPENSNES MAPNOBJ

CSRC        := main.c mario.c goomba.c koopatroopa.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object

include $(OPENSNES)/make/common.mk
