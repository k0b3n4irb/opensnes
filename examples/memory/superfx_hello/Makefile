OPENSNES := $(shell cd ../../.. && pwd)

TARGET      := superfx_hello.sfc
ROM_NAME    := SUPERFX HELLO

CSRC        := main.c
ASMSRC      := gsu_loader.asm
GSUSRC      := gsu_hello.sfx

USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console sprite dma background text superfx

include $(OPENSNES)/make/common.mk
