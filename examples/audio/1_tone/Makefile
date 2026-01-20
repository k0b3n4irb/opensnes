# TADA Sound Example
# Plays TADA sample using the SPC700 DSP

OPENSNES := $(shell cd ../../.. && pwd)

# ROM Configuration
TARGET   := tone.sfc
ROM_NAME := OPENSNES TADA

# Use the library for input handling
USE_LIB     := 1
LIB_MODULES := console input sprite dma

# Source files
CSRC     := main.c
ASMSRC   := spc.asm data.asm

# Include shared build rules
include $(OPENSNES)/make/common.mk
