# Save Game - Multi-slot SRAM save/load demo

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := save_game.sfc
ROM_NAME := OPENSNES SAVE GAME

USE_LIB     := 1
USE_SRAM    := 1
SRAM_SIZE   := 3
LIB_MODULES := console dma text background sprite input

CSRC   := main.c

include $(OPENSNES)/make/common.mk
