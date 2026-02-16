# Minimal smoke test — verifies the toolchain builds a working ROM
OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := minimal.sfc
ROM_NAME   := MINIMAL TEST
CSRC       := main.c
USE_LIB    := 1
LIB_MODULES := console dma

include $(OPENSNES)/make/common.mk
