#==============================================================================
# Map and Objects - Scrolling platformer with object engine
#==============================================================================
# Demonstrates: map engine scrolling, object engine with physics/collision,
#               dynamic sprites, enemy AI (goomba, koopa troopa).
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := platform_objects.sfc
ROM_NAME    := PLATFORM OBJECTS

CSRC        := main.c kit.c insect.c automaton.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object

include $(OPENSNES)/make/common.mk
