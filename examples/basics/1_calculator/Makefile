#==============================================================================
# Calculator Example - Makefile
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := calculator.sfc
ROM_NAME := SNES CALCULATOR

# Source files
CSRC     := main.c

# Enable OpenSNES library
USE_LIB     := 1
LIB_MODULES := console input sprite

include $(OPENSNES)/make/common.mk
