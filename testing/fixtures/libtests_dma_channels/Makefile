# DMA channels the library takes behind the game's back (2026-10-17, audit
# 1.0 finding 01-1): a Super FX letterbox (HDMA channel 1), the text flush
# and the dynamic sprite uploads of the NMI, and the game's own HDMA on
# channels 2-6, together. Asserted by testing/manifests/libtest_dma_channels.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_dma_channels.sfc
ROM_NAME    := LIB TEST DMA CHANNELS
CSRC        := main.c
USE_LIB     := 1
USE_SUPERFX := 1
LIB_MODULES := console dma background text hdma sprite sprite_dynamic
include $(OPENSNES)/make/common.mk
