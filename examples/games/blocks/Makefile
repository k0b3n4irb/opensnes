# Blocks — falling-blocks puzzle, Mode 1, 3-BG architecture (playfield / HUD / messages)
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := blocks.sfc
ROM_NAME := OPENSNES BLOCKS

CSRC     := main.c board.c piece.c render.c hud.c
ASMSRC   := data.asm

USE_LIB  := 1
USE_SNESMOD := 1
LIB_MODULES := console sprite dma background input hdma

# SNESMOD audio
SOUNDBANK_SRC := music/blocks_theme.it

include $(OPENSNES)/make/common.mk
