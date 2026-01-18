#==============================================================================
# Mode 7 Example Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := mode7.sfc
ROM_NAME   := MODE7 DEMO OPENSNES

# Use the OpenSNES library
USE_LIB    := 1
LIB_MODULES := console input sprite mode7

# Source files
CSRC       := main.c
ASMSRC     := data.asm

# Include common build rules
include $(OPENSNES)/make/common.mk
