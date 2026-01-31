#==============================================================================
# Entity System Demo - Makefile
#==============================================================================
# Demonstrates entity management for game objects

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := entity_demo.sfc
ROM_NAME := ENTITY DEMO

# Source files
CSRC     := main.c

# Enable OpenSNES library with entity support
USE_LIB     := 1
LIB_MODULES := console input sprite dma entity

include $(OPENSNES)/make/common.mk
