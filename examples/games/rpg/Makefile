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
res/palplan.h: res/sprites.palplan res/hero.pal res/npc.pal
	@echo "[PAL] planning sprite palettes..."
	@$(PALPLAN) -o $@ $<

# -c imposes the committed palette: a colour that is not in it fails the
# build, instead of the converter quietly re-deriving all 16 slots and
# shifting every tile already drawn.
# -s 16: the sheet holds 16x16 frames, so gfx4snes must cut 16x16
# blocks — see docs/tutorials/sprites.md. Converting it with -s 8
# happens to work only while the sheet is exactly 16 tiles wide.
# -o 16 -u 16: the hero is a 16-colour 4bpp sheet and the game only ever
# loads its first 16 colours into one OBJ slot, so cap the palette at 16.
# (Without the cap gfx4snes writes a full 256-colour .pal — 480 bytes of
# ROM the game never reads, which is also why palplan, which plans
# 16-colour slots, needs the cap here.)
res/interior.pic res/interior.pal: res/interior.png res/interior_fixed.pal
	@echo "[GFX] interior..."
	@$(OPENSNES)/bin/gfx4snes -s 8 -o 16 -u 16 -p -m -c res/interior_fixed.pal -i $<

