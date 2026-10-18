#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

SUBDIRS := sa1-patch opensnes-sample opensnes-music opensnes-rom opensnes-sprite opensnes-tileset opensnes-level opensnes-text opensnes-palette opensnes-image opensnes-save opensnes

.PHONY: all clean install $(SUBDIRS)

all clean install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
