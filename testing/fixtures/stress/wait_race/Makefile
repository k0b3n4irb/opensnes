OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := wait_race.sfc
ROM_NAME := WAIT RACE PROBE
USE_LIB  := 1
LIB_MODULES := console

CSRC   := main.c
ASMSRC := delay.asm    # race_delay: the sweep of the store's phase

include $(OPENSNES)/make/common.mk
