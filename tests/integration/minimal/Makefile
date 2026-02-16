# Minimal OpenSNES Test ROM Makefile
# NOTE: This test uses an outdated custom build process (missing project_hdr.asm).
# TODO: Update to use common.mk like the main examples.

.PHONY: all clean
all:
	@echo "[SKIP] minimal integration test disabled (outdated build)"
clean:
	@echo "[SKIP] Nothing to clean"

# Old build code below - kept for reference
ifdef ENABLE_OLD_BUILD
OPENSNES := $(shell cd ../../.. && pwd)
CC = $(OPENSNES)/bin/cc65816
AS = $(OPENSNES)/bin/wla-65816
LD = $(OPENSNES)/bin/wlalink

ROM = minimal.sfc

old_all: $(ROM)
	@echo "Built: $(ROM)"
	@ls -la $(ROM)

# Compile C to WLA-DX assembly
main.c.asm: main.c
	$(CC) -I$(OPENSNES)/lib/include $< -o $@

# Combine crt0 + main into single assembly (simpler for minimal test)
combined.asm: crt0.asm main.c.asm project_hdr.asm
	@cat crt0.asm > $@
	@echo "" >> $@
	@echo "; === Compiled C code ===" >> $@
	@sed -e '/^\.data/d' \
	     -e '/^\/\* end/d' \
	     -e '/^\.balign/d' \
	     -e '/^\.GLOBAL/d' \
	     -e 's/\.byte/.db/g' \
	     main.c.asm >> $@

# Assemble combined file
combined.obj: combined.asm project_hdr.asm
	$(AS) -o $@ $<

# Link
linkfile: combined.obj
	@echo "[objects]" > $@
	@echo "combined.obj" >> $@

$(ROM): linkfile combined.obj
	$(LD) -v -S $< $@

clean:
	rm -f *.obj *.c.asm combined.asm *.sfc *.sym linkfile

endif
