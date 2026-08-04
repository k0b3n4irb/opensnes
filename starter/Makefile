# OpenSNES starter project
# -----------------------------------------------------------------------------
# Where is your OpenSNES SDK?
#   - Building this copy inside the OpenSNES repo, the default below (the repo
#     root) just works.
#   - After you copy this project OUT to start your own game, point it at your
#     installed SDK — either edit the line below, or build with:
#         make OPENSNES=/path/to/opensnes
#     (Get the SDK from a release zip, or clone + `make` the repo.)
OPENSNES ?= $(shell cd .. && pwd)

TARGET   := game.sfc
ROM_NAME := MY GAME              # up to 21 chars — shown in the ROM header

USE_LIB     := 1
# Link only the modules you use (smaller ROM, faster build). Add as you grow:
#   background text  (tilemaps + on-screen text)
#   audio           (BRR sound effects — see the audio examples)
LIB_MODULES := console dma sprite input

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# -----------------------------------------------------------------------------
# Graphics: res/player.png -> res/player.pic + res/player.pal (gfx4snes).
# A 4bpp 32x32 sprite. Copy this block for each new .png you add.
# (A .wav .incbin'd as its .brr auto-converts too — see the audio tutorial.)
# -----------------------------------------------------------------------------
res/player.pic res/player.pal: res/player.png
	@echo "[GFX] $< -> player.pic/.pal"
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

combined.asm: res/player.pic

clean: clean-assets
.PHONY: clean-assets
clean-assets:
	@rm -f res/player.pic res/player.pal res/player.inc res/player_data.as
