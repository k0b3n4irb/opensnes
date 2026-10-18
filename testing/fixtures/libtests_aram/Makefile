# The sound CPU's memory (2026-10-13): a sample loaded in steps, and the echo
# ring's place (the top delay x 2 KB and no more). Its own fixture because the
# proofs need 48 KB of samples loaded and read the memory back, which the
# shared fixture's ARAM vectors could not survive.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := libtest_aram.sfc
ROM_NAME := LIB RUNTIME ARAM
USE_LIB  := 1
LIB_MODULES := console dma audio
CSRC   := main.c
ASMSRC := data.asm
include $(OPENSNES)/make/common.mk
