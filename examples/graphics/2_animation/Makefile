# Animated Sprite Example
# Port of pvsneslib AnimatedSprite

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := animation.sfc
ROM_NAME := OPENSNES ANIMATION

# Enable OpenSNES library
USE_LIB     := 1
LIB_MODULES := console sprite input

# Graphics conversion
GFXSRC      := assets/sprites.bmp
SPRITE_SIZE := 16
BPP         := 4

# Additional assembly
ASMSRC := helpers.asm

# Include shared build rules
include $(OPENSNES)/make/common.mk
