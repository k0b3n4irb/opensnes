#==============================================================================
# Slope Mario - Platformer with slope collision
#==============================================================================
# Demonstrates: objCollidMapWithSlopes for diagonal terrain collision,
#               map engine scrolling, dynamic sprites, player physics.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := slope_collision.sfc
ROM_NAME    := OPENSNES SLOPMARIO

CSRC        := main.c mario.c
ASMSRC      := data.asm

USE_LIB     := 1
LIB_MODULES := console sprite sprite_dynamic sprite_lut dma input background map object

include $(OPENSNES)/make/common.mk
