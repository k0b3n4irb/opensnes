# The echo ring's place in the sound CPU's memory (2026-10-13): it takes the
# top delay x 2 KB and no more. Its own fixture because the proof needs 48 KB
# of samples loaded, which the shared fixture's ARAM vectors could not survive.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := libtest_echo.sfc
ROM_NAME := LIB RUNTIME ECHO
USE_LIB  := 1
LIB_MODULES := console dma audio
CSRC   := main.c
ASMSRC := data.asm
include $(OPENSNES)/make/common.mk
