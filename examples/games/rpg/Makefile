OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := rpg.sfc
ROM_NAME := RPG TEMPLATE
USE_LIB  := 1
LIB_MODULES := console dma background sprite text input collision panel

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# AFTER the include (default-goal rule). main.c #includes the generated
# entity defines of the Tiled map (gen_assets.py). Without this, editing the
# map and re-running gen_assets.py relinks the ROM with a STALE C object —
# the map moves, the spawn does not. The sprite-palette plan
# (res/palettes.toml, opensnes-palette) and the pictures convert through
# make/common.mk's generic rule, the plan after the pictures.
main.c.o: res/entities.inc
