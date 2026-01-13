#==============================================================================
# OpenSNES Minimal Template
#==============================================================================
#
# Usage:
#   1. Copy this directory to your projects folder
#   2. Update OPENSNES path below to point to your SDK installation
#   3. Change TARGET and ROM_NAME for your project
#   4. Run 'make' to build
#
# Commands:
#   make        - Build the ROM
#   make clean  - Remove build files
#   make run    - Build and run in Mesen2 (if installed)
#
#==============================================================================

# Path to OpenSNES SDK (adjust for your system)
# Option 1: Relative path (if this project is inside opensnes/projects/)
OPENSNES := $(shell cd ../.. && pwd)
# Option 2: Absolute path (uncomment and edit)
# OPENSNES := /path/to/opensnes

# ROM Configuration
TARGET   := mygame.sfc
ROM_NAME := MY SNES GAME

# Source files (add more .c files as needed)
CSRC     := main.c

# Include the build system (don't modify this line)
include $(OPENSNES)/make/common.mk

#==============================================================================
# Custom targets (optional)
#==============================================================================

# Run in emulator
.PHONY: run
run: $(TARGET)
	@echo "Starting emulator..."
	@mesen $(TARGET) 2>/dev/null || open $(TARGET) 2>/dev/null || echo "No emulator found. Open $(TARGET) manually."
