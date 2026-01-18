#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

SUBDIRS := font2snes gfx2snes smconv

.PHONY: all clean install $(SUBDIRS)

all clean install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
