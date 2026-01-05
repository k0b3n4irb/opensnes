# Minimal OpenSNES Test ROM Makefile

OPENSNES := $(shell cd ../../.. && pwd)
CC = $(OPENSNES)/bin/cc65816
AS = $(OPENSNES)/bin/wla-65816
LD = $(OPENSNES)/bin/wlalink

ROM = minimal.sfc

.PHONY: all clean

all: $(ROM)
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
