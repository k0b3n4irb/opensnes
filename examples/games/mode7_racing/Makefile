OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := mode7_racing.sfc
ROM_NAME := MODE7 RACING
USE_LIB  := 1
LIB_MODULES := console dma background sprite math mode7 input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# AFTER the include on purpose: the first rule in a Makefile is the
# default goal (common.mk provides `all`). One gfx4snes run produces
# all three outputs — list them all (data.o depends on each via
# INCBIN_DEPS; a single-target rule left .mp7/.pal ruleless on fresh
# checkouts and broke CI).
res/track.pc7 res/track.mp7 res/track.pal: res/track.png
	@echo "[GFX] Converting Mode 7 track..."
	@$(OPENSNES)/bin/gfx4snes -p -m -M 7 -i $<

clean: clean-track
.PHONY: clean-track
clean-track:
	@rm -f res/track.pc7 res/track.mp7 res/track.pal res/track.inc res/track_data.as
