OPENSNES := $(shell cd ../../.. && pwd)

TARGET   := rpg.sfc
ROM_NAME := RPG TEMPLATE
USE_LIB  := 1
LIB_MODULES := console dma background sprite text input collision panel

CSRC   := main.c
ASMSRC := data.asm

include $(OPENSNES)/make/common.mk

# AFTER the include (default-goal rule). The terrain tileset, the hero
# sheet and the dialog-box tiles are converted from the committed PNGs;
# the map binaries come from res/town.tmj via gen_assets.py.
# main.c #includes the generated entity defines. Without this, editing
# the map and re-running gen_assets.py relinks the ROM with a STALE C
# object — the map moves, the spawn does not.
main.c.o: res/entities.inc res/palplan.h

# palplan assigns the sprite palettes their CGRAM slots and writes the
# offsets main.c uses (PAL_HERO_CGRAM / PAL_NPC_CGRAM and the matching
# _SLOT for the OAM palette field). Regenerated whenever a sprite palette
# changes, so adding an NPC never desyncs the code from the plan. The plan
# table prints during the build.
# res/hero.pal comes out of res/hero.png.toml (opensnes-sprite): wait for that
# conversion, or a parallel clean build has no rule for it (CI, 2026-10-05).
res/hero.pal: | res/hero.png.toml.done
res/palplan.h: res/sprites.palplan res/hero.pal res/npc.pal
	@echo "[PAL] planning sprite palettes..."
	@$(PALPLAN) -o $@ $<
