#==============================================================================
# shmup_1942 — S1: Static tile library preview
#
# Vertical 1942-style shoot 'em up, built on Kenney's CC0 Pixel Shmup pack.
# S1 just proves the gfx4snes → bgLoad pipeline by displaying the
# ground tile library on BG1 with no scrolling and no sprites.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := shmup_1942.sfc
ROM_NAME   := SHMUP 1942

USE_LIB    := 1
LIB_MODULES := console dma background asset sprite input

CSRC       := main.c
include $(OPENSNES)/make/common.mk
