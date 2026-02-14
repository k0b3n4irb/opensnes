# PVSnesLib Examples - Ported to OpenSNES
#
# These examples are ported from PVSnesLib to validate the OpenSNES SDK.
# Each example uses only the OpenSNES API — no PVSnesLib compatibility layer.
#
# Auto-discovers all subdirectories containing a Makefile.

EXAMPLES := $(sort $(patsubst %/Makefile,%,$(shell find . -mindepth 2 -name Makefile -printf '%P\n')))

.PHONY: all clean $(EXAMPLES)

all: $(EXAMPLES)

$(EXAMPLES):
	@echo "=== Building $@ ==="
	@$(MAKE) -C $@

clean:
	@for dir in $(EXAMPLES); do \
		echo "=== Cleaning $$dir ==="; \
		$(MAKE) -C $$dir clean; \
	done
