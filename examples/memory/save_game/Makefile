#==============================================================================
# Save Game Example - Makefile
#==============================================================================
# Demonstrates SRAM save/load functionality

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := save_game.sfc
ROM_NAME := SRAM SAVE EXAMPLE

# Source files
CSRC     := main.c

# Enable OpenSNES library (sprite+dma required for NMI handler)
USE_LIB     := 1
LIB_MODULES := console sprite dma

# Enable SRAM for save games (8KB)
USE_SRAM    := 1
SRAM_SIZE   := 3

include $(OPENSNES)/make/common.mk
