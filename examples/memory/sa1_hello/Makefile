# SA-1 Hello World Example
# Verifies SA-1 cartridge type is detected by emulator

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sa1_hello.sfc
ROM_NAME := SA1 HELLO WORLD

CSRC     := main.c

USE_LIB  := 1
USE_SA1  := 1
LIB_MODULES := console sprite dma background text input

include $(OPENSNES)/make/common.mk
