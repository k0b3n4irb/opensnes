#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

SUBDIRS := aseprite2snes font2snes gfx4snes img2snes palplan sa1-patch smconv tmx2snes wav2brr

.PHONY: all clean install $(SUBDIRS)

all clean install: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
