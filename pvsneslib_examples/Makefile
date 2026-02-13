# PVSnesLib Examples - Ported to OpenSNES
#
# These examples are ported from PVSnesLib to validate the OpenSNES SDK.
# Each example uses only the OpenSNES API — no PVSnesLib compatibility layer.

EXAMPLES := \
	hello_world \
	random \
	typeconsole \
	input/controller \
	timer \
	testregion \
	breakpoints \
	debug \
	scoring \
	arithmetic \
	memory_mapping \
	benchmark \
	sram/sramsimple \
	sram/sramoffset \
	audio/tada \
	audio/music \
	audio/music2 \
	audio/effects \
	graphics/palette/getcolors \
	graphics/sprites/objectsize \
	graphics/sprites/dynamicsprite \
	graphics/sprites/metasprite \
	graphics/sprites/minimal_sprite32 \
	graphics/sprites/sprite32_diag \
	graphics/backgrounds/mode1bg3hp \
	graphics/backgrounds/mode1scroll \
	graphics/backgrounds/mode7 \
	graphics/effects/gradientcolors \
	graphics/effects/hdmagradient \
	graphics/effects/mosaicshading \
	graphics/effects/parallaxscrolling \
	graphics/effects/transparency \
	graphics/effects/transparentwindow \
	graphics/effects/waves \
	graphics/effects/window

.PHONY: all clean $(EXAMPLES)

all: $(EXAMPLES)

$(EXAMPLES):
	@echo "=== Building $@ ==="
	$(MAKE) -C $@

clean:
	@for dir in $(EXAMPLES); do \
		echo "=== Cleaning $$dir ==="; \
		$(MAKE) -C $$dir clean; \
	done
