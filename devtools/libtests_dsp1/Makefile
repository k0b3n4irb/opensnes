# Third library fixture (coverage lot D, 2026-09-20): the DSP-1 commands no
# example calls. USE_DSP1 changes the cartridge header (chip type), so this
# cannot live in the other fixtures; and the chip needs the user-supplied
# firmware dump, so its test SKIPs cleanly where that is absent (CI).
OPENSNES := $(shell cd ../.. && pwd)
TARGET   := libtest_dsp1.sfc
ROM_NAME := LIB RUNTIME DSP1
USE_LIB  := 1
USE_DSP1 := 1
LIB_MODULES := console dma
CSRC   := main.c
include $(OPENSNES)/make/common.mk
