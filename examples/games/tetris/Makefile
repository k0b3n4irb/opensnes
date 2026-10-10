# Tetris — Mode 1, 3-BG architecture (playfield / HUD / messages)
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := tetris.sfc
ROM_NAME := OPENSNES TETRIS

CSRC     := main.c board.c piece.c render.c hud.c
ASMSRC   := data.asm

USE_LIB  := 1
USE_SNESMOD := 1
LIB_MODULES := console sprite dma background input

# SNESMOD audio
SOUNDBANK_SRC := music/tetris_theme.it

include $(OPENSNES)/make/common.mk
