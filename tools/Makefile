#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

SUBDIRS := font2snes gfx4snes img2snes smconv tmx2snes

.PHONY: all clean install $(SUBDIRS)

all clean install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
