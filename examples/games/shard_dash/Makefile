# shard_dash — a game in several files: one C file per screen, a screen layer, a versioned save
OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := shard_dash.sfc
ROM_NAME := SHARD DASH
USE_LIB  := 1
USE_SRAM := 1
SRAM_SIZE := 3
LIB_MODULES := console dma background sprite text input vramqueue debug sram

CSRC := main.c screen.c ui.c save.c title.c play.c result.c

include $(OPENSNES)/make/common.mk
