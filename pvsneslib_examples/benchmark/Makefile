# Benchmark - PVSnesLib example ported to OpenSNES
# Math benchmark using frame counting to measure operation speed

OPENSNES := $(shell cd ../.. && pwd)

TARGET   := benchmark.sfc
ROM_NAME := OPENSNES BENCHMARK

USE_LIB  := 1
LIB_MODULES := console dma text background sprite

CSRC     := main.c

include $(OPENSNES)/make/common.mk
