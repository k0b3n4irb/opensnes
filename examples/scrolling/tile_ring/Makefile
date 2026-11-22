OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := tile_ring.sfc
ROM_NAME   := TILE RING

USE_LIB    := 1
LIB_MODULES := console background input tilering vramqueue
# A frame in direct mode queues one entry per run of characters: room for a
# row and a column of them, and the four tilemap entries.
VRAM_QUEUE_MAX := 72

CSRC       := main.c

include $(OPENSNES)/make/common.mk
