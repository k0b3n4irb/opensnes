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
main.c.o: res/entities.inc

# -c imposes the committed palette: a colour that is not in it fails the
# build, instead of the converter quietly re-deriving all 16 slots and
# shifting every tile already drawn.
res/tileset.pic res/tileset.pal: res/tileset.png res/town_fixed.pal
	@echo "[GFX] tileset..."
	@$(OPENSNES)/bin/gfx4snes -s 8 -o 16 -u 16 -p -m -c res/town_fixed.pal -i $<
# -s 16: the sheet holds 16x16 frames, so gfx4snes must cut 16x16
# blocks — see docs/tutorials/sprites.md. Converting it with -s 8
# happens to work only while the sheet is exactly 16 tiles wide.
res/hero.pic res/hero.pal: res/hero.png
	@echo "[GFX] hero..."
	@$(OPENSNES)/bin/gfx4snes -s 16 -p -i $<
res/interior.pic res/interior.pal: res/interior.png res/interior_fixed.pal
	@echo "[GFX] interior..."
	@$(OPENSNES)/bin/gfx4snes -s 8 -o 16 -u 16 -p -m -c res/interior_fixed.pal -i $<
res/uibox.pic res/uibox.pal: res/uibox.png
	@echo "[GFX] uibox..."
	@$(OPENSNES)/bin/gfx4snes -s 8 -o 16 -u 16 -p -m -i $<

clean: clean-gfx
.PHONY: clean-gfx
clean-gfx:
	@rm -f res/tileset.pic res/tileset.pal res/tileset.inc res/tileset_data.as res/tileset.map \
	       res/hero.pic res/hero.pal res/hero.inc res/hero_data.as \
	       res/interior.pic res/interior.pal res/interior.inc res/interior_data.as res/interior.map \
	       res/uibox.pic res/uibox.pal res/uibox.inc res/uibox_data.as
