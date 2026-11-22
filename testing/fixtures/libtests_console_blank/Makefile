# consoleInit() after setScreenOn() (2026-11-22): the force-blank shadow
# follows the screen, so the DMA screen check raises no false alarm.
# Asserted by testing/manifests/libtest_console_blank.toml.
OPENSNES := $(shell cd ../../.. && pwd)
TARGET      := libtest_console_blank.sfc
ROM_NAME    := LIB TEST CONS BLANK
CSRC        := main.c
USE_LIB     := 1
LIB_MODULES := console dma text background
include $(OPENSNES)/make/common.mk
