OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := audio_done.sfc
ROM_NAME := AUDIO DONE PROBE
USE_LIB  := 1
LIB_MODULES := console audio profile

CSRC   := main.c

# make clean && make FAULT=1: audioCommandDone() replaced by one that always
# reads 1 (main.c), to show that audio_done.toml catches it
ifeq ($(FAULT),1)
CFLAGS += -DDONE_FAULT
endif

include $(OPENSNES)/make/common.mk
