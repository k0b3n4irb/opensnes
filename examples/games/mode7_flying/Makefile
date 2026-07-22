OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := mode7_flying.sfc
ROM_NAME := MODE7 FLYING
USE_LIB  := 1
LIB_MODULES := console dma background sprite math mode7 input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# AFTER the include (default-goal rule); ALL THREE outputs declared
# (the mode7_racing CI lesson: data.o depends on each via INCBIN_DEPS).
res/terrain.pc7 res/terrain.mp7 res/terrain.pal: res/terrain.png
	@echo "[GFX] Converting Mode 7 terrain..."
	@$(OPENSNES)/bin/gfx4snes -p -m -M 7 -i $<

clean: clean-terrain
.PHONY: clean-terrain
clean-terrain:
	@rm -f res/terrain.pc7 res/terrain.mp7 res/terrain.pal res/terrain.inc res/terrain_data.as
