#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

SUBDIRS := font2snes gfx4snes img2snes sa1-patch smconv tmx2snes

.PHONY: all clean install $(SUBDIRS)

all clean install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
