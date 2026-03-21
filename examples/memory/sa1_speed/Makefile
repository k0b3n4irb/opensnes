# SA-1 Speed Demo — Shows 10.74 MHz coprocessor speed

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sa1_speed.sfc
ROM_NAME := SA1 SPEED DEMO

CSRC     := main.c

USE_LIB  := 1
USE_SA1  := 1
LIB_MODULES := console sprite dma background text input sa1

include $(OPENSNES)/make/common.mk
