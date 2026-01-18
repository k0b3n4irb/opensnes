# Sound Effects Demo
# Uses the OpenSNES audio library

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := sfx_demo.sfc
ROM_NAME := OPENSNES SFX DEMO

# Source files
CSRC     := main.c
ASMSRC   := data.asm

# Use library modules (console and sprite are required by crt0.asm)
USE_LIB     := 1
LIB_MODULES := console sprite audio

# Include shared build rules
include $(OPENSNES)/make/common.mk
