# Hello World Example
# Uses OpenSNES library for console functions

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := hello_world.sfc
ROM_NAME := OPENSNES HELLO WORLD

# Use OpenSNES library
USE_LIB  := 1

# Library modules to link
LIB_MODULES := console sprite

# Source files
CSRC     := main.c

# Include shared build rules
include $(OPENSNES)/make/common.mk
