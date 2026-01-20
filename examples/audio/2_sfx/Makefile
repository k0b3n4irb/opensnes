# Sound Effects Demo
# Uses library for input, bare-metal SPC700 for audio

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := sfx_demo.sfc
ROM_NAME := OPENSNES SFX DEMO

# Use the library for input handling
USE_LIB     := 1
LIB_MODULES := console input sprite dma

# Source files
CSRC     := main.c
ASMSRC   := spc.asm

# Include shared build rules
include $(OPENSNES)/make/common.mk
