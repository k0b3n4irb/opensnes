# sfx_from_wav — one-shot sound effects from WAV files (res/*.wav.toml, opensnes-sample)

OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := sfx_from_wav.sfc
ROM_NAME := SFX FROM WAV

USE_LIB     := 1
LIB_MODULES := console dma audio input background text

CSRC   := main.c
include $(OPENSNES)/make/common.mk
