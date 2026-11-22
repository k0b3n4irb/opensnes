# One build of the bench; run.py builds it under build/placebench/ and passes
# OPENSNES (and USE_FASTROM for the FastROM column).
TARGET   := placebench.sfc
ROM_NAME := PLACEBENCH
USE_LIB  := 1
LIB_MODULES := console sprite
CSRC     := bench.c
ASMSRC   := kernels.asm place_old.asm
include $(OPENSNES)/make/common.mk
