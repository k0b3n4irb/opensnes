OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := koff_hold.sfc
ROM_NAME := KOFF HOLD PROBE
USE_LIB  := 1
LIB_MODULES := console audio profile

CSRC   := main.c
ASMSRC := delay.asm    # koff_delay: the sweep of the key-off's phase

include $(OPENSNES)/make/common.mk
