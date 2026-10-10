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

include $(OPENSNES)/make/common.mk

# -----------------------------------------------------------------------------
# Graphics: res/player.png is converted by opensnes-sprite before the build
# compiles anything, from the settings beside it (res/player.png.toml), and
# its .incbin lands in the generated assets_gen.asm. Add a .png and its .toml
# in res/ and you are done — no rule here, no data.asm.
# -----------------------------------------------------------------------------
