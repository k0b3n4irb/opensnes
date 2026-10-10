OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := aseprite_pipeline.sfc
ROM_NAME := ASEPRITE PIPELINE

CSRC     := main.c

USE_LIB  := 1
LIB_MODULES := console sprite dma text text4bpp background input anim

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Assets: res/hero.png.toml and res/hero.json.toml say how the sheet and the
# Aseprite export are converted (opensnes-sprite sheet / anim); the build
# runs them before compiling and gathers the .incbin into assets_gen.asm.
# No rule here, no data.asm (docs/tools/CONVENTIONS.md).
#------------------------------------------------------------------------------
