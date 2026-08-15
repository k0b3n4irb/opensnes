OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := aseprite_pipeline.sfc
ROM_NAME := ASEPRITE PIPELINE

CSRC     := main.c
ASMSRC   := data.asm

USE_LIB  := 1
LIB_MODULES := console sprite dma text text4bpp background input anim

include $(OPENSNES)/make/common.mk

#------------------------------------------------------------------------------
# Asset generation — the full Aseprite sprite pipeline
#
#   gfx4snes -P : hero.png sheet -> tiles + palette + metasprite pointer table
#                 (-T transposes tiles into SNES OBJ VRAM grid layout;
#                  -X/-Y give the 32x48 metasprite size; -P 2 sets priority 2)
#   aseprite2snes: hero.json     -> AnimClip tables, one per Aseprite tag
#------------------------------------------------------------------------------
res/hero.pic res/hero.pal res/hero_meta.inc: res/hero.png
	$(GFX4SNES) -s 16 -o 16 -u 16 -p -T -X 32 -Y 48 -P 2 -i $<

res/hero_anim.h: res/hero.json
	$(ASEPRITE2SNES) -o $@ -p hero $<

# main.c #includes the generated metasprite table + anim clips, so it must
# wait for both generators. (data.asm's .incbin of hero.pic is auto-tracked.)
main.c.o: res/hero_meta.inc res/hero_anim.h

clean: clean-aseprite_pipeline
.PHONY: clean-aseprite_pipeline
clean-aseprite_pipeline:
	@rm -f res/hero.pic res/hero.pal res/hero.inc res/hero_data.as res/hero_meta.inc res/hero_anim.h
