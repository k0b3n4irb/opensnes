#==============================================================================
# OpenSNES Game Template
#==============================================================================
#
# A structured game template with:
# - State management (title, playing, paused)
# - Sprite-based player
# - Background scrolling
# - Input with edge detection
#
# Commands:
#   make        - Build the ROM
#   make clean  - Remove build files
#   make run    - Build and run in Mesen2
#
#==============================================================================

# Path to OpenSNES SDK (adjust for your system)
OPENSNES := $(shell cd ../.. && pwd)

# ROM Configuration
TARGET   := mygame.sfc
ROM_NAME := MY SNES GAME

# Source files
CSRC     := main.c

# Include the build system
include $(OPENSNES)/make/common.mk

#==============================================================================
# Custom targets
#==============================================================================

.PHONY: run
run: $(TARGET)
	@echo "Starting emulator..."
	@mesen $(TARGET) 2>/dev/null || open $(TARGET) 2>/dev/null || echo "Open $(TARGET) manually."
