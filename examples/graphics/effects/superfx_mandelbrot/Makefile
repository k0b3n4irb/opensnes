OPENSNES := $(shell cd ../../../.. && pwd)

TARGET      := superfx_mandelbrot.sfc
ROM_NAME    := SUPERFX MANDELBROT   

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_mandelbrot.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background input superfx

include $(OPENSNES)/make/common.mk
