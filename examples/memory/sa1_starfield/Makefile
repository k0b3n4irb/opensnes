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
