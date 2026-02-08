# PVSnesLib Examples - Ported to OpenSNES
#
# These examples are ported from PVSnesLib to validate the OpenSNES SDK.
# Each example uses only the OpenSNES API — no PVSnesLib compatibility layer.

EXAMPLES := hello_world random typeconsole input/controller

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
