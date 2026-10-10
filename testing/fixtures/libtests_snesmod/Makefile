OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := libtest_snesmod.sfc
ROM_NAME := LIB RUNTIME SNESMOD

USE_LIB     := 1
USE_SNESMOD := 1
# No soundbank: the fixture only talks to the driver's command queue.
LIB_MODULES := console profile

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
