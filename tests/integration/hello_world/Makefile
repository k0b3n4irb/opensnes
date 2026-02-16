# Hello World integration test — verifies text module and library linking
OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := hello_world.sfc
ROM_NAME   := HELLO WORLD TEST
CSRC       := main.c
USE_LIB    := 1
LIB_MODULES := console dma text

include $(OPENSNES)/make/common.mk
