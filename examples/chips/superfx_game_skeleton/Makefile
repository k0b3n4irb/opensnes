OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := superfx_game_skeleton.sfc
ROM_NAME    := SUPERFX GAME SKELETON

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_cube.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background input   # superfx auto-added by USE_SUPERFX

include $(OPENSNES)/make/common.mk
