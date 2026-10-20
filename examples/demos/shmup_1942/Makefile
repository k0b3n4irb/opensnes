#==============================================================================
# shmup_1942 — a vertical 1942-style shoot 'em up
#
# Vertical 1942-style shoot 'em up, built on Kenney's CC0 Pixel Shmup pack.
# A scrolling ground on BG1, the player's plane, enemies and bullets as
# sprites; every picture is converted by the build from its settings file.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := shmup_1942.sfc
ROM_NAME   := SHMUP 1942

USE_LIB    := 1
LIB_MODULES := console dma background asset sprite input

CSRC       := main.c
include $(OPENSNES)/make/common.mk
