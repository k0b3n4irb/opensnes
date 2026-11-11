# One build of the bench; run.py builds it under build/gapbench/ and passes
# OPENSNES (and USE_FASTROM for the FastROM column).
TARGET   := gapbench.sfc
ROM_NAME := GAPBENCH
USE_LIB  := 1
LIB_MODULES := console string
CSRC     := kernels.c
ASMSRC   := kernels.asm
include $(OPENSNES)/make/common.mk
