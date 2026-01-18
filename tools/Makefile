#==============================================================================
# OpenSNES Tools - Makefile
#==============================================================================

# Core tools (always built)
CORE_SUBDIRS := font2snes gfx2snes

# Optional tools (may require additional dependencies)
# smconv requires Go - built separately if available
OPTIONAL_SUBDIRS := smconv

.PHONY: all clean install $(CORE_SUBDIRS) smconv-optional

all: $(CORE_SUBDIRS) smconv-optional

clean:
	@for dir in $(CORE_SUBDIRS) $(OPTIONAL_SUBDIRS); do \
		$(MAKE) -C $$dir clean 2>/dev/null || true; \
	done

install: $(CORE_SUBDIRS) smconv-optional

$(CORE_SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

# Build smconv if Go is available, otherwise skip with a warning
smconv-optional:
	@if which go > /dev/null 2>&1; then \
		$(MAKE) -C smconv $(MAKECMDGOALS); \
	else \
		echo "[SKIP] smconv - Go not installed (optional, needed for SNESMOD)"; \
	fi
