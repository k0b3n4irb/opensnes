OPENSNES := $(shell cd ../../../.. && pwd)

TARGET      := superfx_3d.sfc
ROM_NAME    := SUPERFX 3D CUBE

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_3d.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background input superfx

include $(OPENSNES)/make/common.mk
