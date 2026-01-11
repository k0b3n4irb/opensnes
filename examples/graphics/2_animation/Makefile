# Animated Sprite Example
# Port of pvsneslib AnimatedSprite

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := animation.sfc
ROM_NAME := OPENSNES ANIMATION

# Graphics conversion
GFXSRC      := assets/sprites.png
SPRITE_SIZE := 16
BPP         := 4

# Additional assembly
ASMSRC := helpers.asm

# Include shared build rules
include $(OPENSNES)/make/common.mk
