#==============================================================================
# shmup_1942 — S1: Static tile library preview
#
# Vertical 1942-style shoot 'em up, built on Kenney's CC0 Pixel Shmup pack.
# S1 just proves the gfx4snes → bgLoad pipeline by displaying the
# ground tile library on BG1 with no scrolling and no sprites.
#==============================================================================

OPENSNES := $(shell cd ../../.. && pwd)

TARGET     := shmup_1942.sfc
ROM_NAME   := SHMUP 1942

USE_LIB    := 1
LIB_MODULES := console dma background asset sprite input text

CSRC       := main.c
ASMSRC     := data.asm

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Graphics conversion
#------------------------------------------------------------------------------
# scene.png — 256×512 procedurally-generated terrain (32×64 tilemap)
# player.png — 32×32 player ship extracted from Kenney's ships_packed.png
# (see res/compose_scene.py and res/extract_player.py for the upstream
# generation).

res/scene.pic res/scene.pal res/scene.map: res/scene.png
	@echo "[GFX] Converting scene tileset..."
	@$(GFX4SNES) -s 8 -o 16 -u 16 -p -m -i $<

res/player.pic res/player.pal: res/player.png
	@echo "[GFX] Converting player sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

res/enemy.pic res/enemy.pal: res/enemy.png
	@echo "[GFX] Converting enemy sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

res/bullet.pic res/bullet.pal: res/bullet.png
	@echo "[GFX] Converting bullet sprite..."
	@$(GFX4SNES) -s 32 -o 16 -u 16 -p -i $<

combined.asm: res/scene.pic res/player.pic res/enemy.pic res/bullet.pic

clean: clean-shmup_1942
.PHONY: clean-shmup_1942
clean-shmup_1942:
	@rm -f res/scene.pic res/scene.pal res/scene.map res/scene.inc res/scene_data.as
	@rm -f res/player.pic res/player.pal res/player.inc res/player_data.as
	@rm -f res/enemy.pic res/enemy.pal res/enemy.inc res/enemy_data.as
	@rm -f res/bullet.pic res/bullet.pal res/bullet.inc res/bullet_data.as
