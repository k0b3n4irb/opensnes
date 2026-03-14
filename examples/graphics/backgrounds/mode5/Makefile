OPENSNES := $(shell cd ../../../.. && pwd)

TARGET     := mode5.sfc
ROM_NAME   := MODE 5 HIRES DEMO

USE_LIB    := 1
LIB_MODULES := console dma background

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

# Mode 5 hi-res conversion: -M 5 for special interleaving
res/bg.pic res/bg.map res/bg.pal: res/bg.png
	@echo "[GFX] $< -> Mode 5 assets"
	@$(GFX4SNES) -i $< -m -u 16 -M 5

combined.asm: res/bg.pic res/bg.map res/bg.pal
