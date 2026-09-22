OPENSNES := $(shell cd ../.. && pwd)

TARGET   := libtest.sfc
ROM_NAME := LIB RUNTIME TEST

USE_LIB  := 1
# L2c (2026-09-15): collision, window and sram get their runtime asserts
# here; USE_SRAM maps bank $70 in the header (common.mk adds the module).
USE_SRAM := 1
# Bank $00 free space is a packing artefact here, not a budget: every section
# of this fixture is SUPERFREE, so wlalink fills bank $00 to the brim and
# spills the rest elsewhere — the figure has read 71, 29 and 13 bytes on
# successive vectors without anything being wrong. The ratchet exists for
# content FORCED into bank $00; with none, it could only fail at random.
BANK0_FAIL_THRESHOLD := 0
LIB_MODULES := console sprite dma background text math anim map audio fixed32 collision window input object colormath mosaic profile scene

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk
