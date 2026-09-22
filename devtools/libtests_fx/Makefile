# Second library fixture (coverage lot C, 2026-09-20): the modules the first
# fixture has no bank-$00 RAM for — hdma (1346 + 113 bytes of tables) and
# mode7 (a 256-byte sine table) — and SNESMOD, which cannot share a ROM with
# the audio v2 driver the first fixture boots. The soundbank is the
# snesmod_music example's module: a fixture needs a real one, not its own.
OPENSNES := $(shell cd ../.. && pwd)
TARGET   := libtest_fx.sfc
ROM_NAME := LIB RUNTIME TEST FX
USE_LIB  := 1
USE_SNESMOD   := 1
SOUNDBANK_SRC := $(OPENSNES)/examples/audio/snesmod_music/music/pollen8.it
LIB_MODULES := console dma hdma mode7 sa1   # sa1: the presence getter on a plain LoROM
CSRC   := main.c
include $(OPENSNES)/make/common.mk
