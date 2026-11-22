OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := audio_nmi.sfc
ROM_NAME := AUDIO NMI PROBE
USE_LIB  := 1
LIB_MODULES := console audio

CSRC   := main.c

# make clean && make FAULT=1: a second sender (the main thread stops every
# voice while the callback sends), to show that audio_nmi.toml catches it
ifeq ($(FAULT),1)
CFLAGS += -DNMI_FAULT
endif

include $(OPENSNES)/make/common.mk
