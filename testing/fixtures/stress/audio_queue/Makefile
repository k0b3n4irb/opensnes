OPENSNES := $(shell cd ../../../.. && pwd)

TARGET   := audio_queue.sfc
ROM_NAME := AUDIO QUEUE PROBE
USE_LIB  := 1
LIB_MODULES := console audio profile

CSRC   := main.c

# make clean && make FAULT=1: the drain ignores the port lock (main.c), to
# show that audio_queue.toml catches it
ifeq ($(FAULT),1)
CFLAGS += -DQUEUE_FAULT
endif

include $(OPENSNES)/make/common.mk
