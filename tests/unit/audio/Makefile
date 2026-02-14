# Audio Unit Test Makefile
#
# NOTE: The audio module's RAMSECTION (.audio_ram at $0400 FORCE, 512 bytes)
# conflicts with crt0's oamMemory mirror ($0300-$051F). We cannot link both.
# This test validates compile-time constants and struct layouts only.

OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := test_audio.sfc
ROM_NAME := AUDIO TESTS

CSRC     := test_audio.c
USE_LIB  := 1
LIB_MODULES := console dma text sprite

include $(OPENSNES)/make/common.mk
