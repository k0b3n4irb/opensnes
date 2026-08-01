# Fundamentals — how a glyph is drawn: hand-coded 2bpp font, direct VRAM writes

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := text_glyphs.sfc
ROM_NAME := TEXT GLYPHS

# Use OpenSNES library
USE_LIB  := 1

# Library modules to link
LIB_MODULES := console sprite dma background

# Source files
CSRC     := main.c

# Include shared build rules
include $(OPENSNES)/make/common.mk
