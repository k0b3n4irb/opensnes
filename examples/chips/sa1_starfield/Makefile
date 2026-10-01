# SA-1 Starfield 3D Demo — 128 stars, perspective projection at 10.74 MHz

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sa1_starfield.sfc
ROM_NAME := SA1 STARFIELD 3D

CSRC     := main.c

USE_LIB     := 1
USE_SA1     := 1
USE_FASTROM := 1
LIB_MODULES := console sprite dma background input sa1

include $(OPENSNES)/make/common.mk

# Where the SA-1 code runs: IRAM (default: the loop and its table are
# copied to I-RAM at boot) or ROM (as until 2026-09-27; the SA-1 then shares
# the ROM with the main CPU). `make clean && make SA1_CODE_IN=ROM` builds
# the ROM version for a luna profile comparison (docs/tutorials/sa1.md).
SA1_CODE_IN ?= IRAM
ASFLAGS += $(if $(filter ROM,$(SA1_CODE_IN)),-D SA1_CODE_IN_ROM)
