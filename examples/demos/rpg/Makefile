OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := rpg.sfc
ROM_NAME := RPG TEMPLATE
USE_LIB  := 1
LIB_MODULES := console dma background sprite text input collision panel

CSRC   := main.c

include $(OPENSNES)/make/common.mk

# AFTER the include (default-goal rule). main.c #includes the entity defines
# opensnes-level writes from the Tiled maps; without this, editing a map
# relinks the ROM with a STALE C object: the map moves, the spawn does not.
# It depends on the stamps, not on the .inc files: no rule names those, the
# tools write them beside the stamp, so on a fresh checkout make would stop
# at "no rule to make target".
# The pictures, the maps and the sprite-palette plan (res/palettes.toml,
# opensnes-palette) convert through make/common.mk's generic rules, the
# maps and the plan after the pictures.
main.c.o: res/town.tmj.toml.done res/house.tmj.toml.done res/palettes.toml.done
