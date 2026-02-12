# Scoring - PVSnesLib example ported to OpenSNES
# Demonstrates the scoring module (scoMemory, scoreAdd, scoreCmp)

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := scoring.sfc
ROM_NAME := OPENSNES SCORING

USE_LIB  := 1
LIB_MODULES := console dma text background sprite scoring

CSRC     := main.c

include $(OPENSNES)/make/common.mk
