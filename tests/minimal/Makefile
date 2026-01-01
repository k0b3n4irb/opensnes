# Minimal OpenSNES Test ROM Makefile

OPENSNES = ../..
CC = $(OPENSNES)/bin/816-tcc
AS = $(OPENSNES)/bin/wla-65816
LD = $(OPENSNES)/bin/wlalink

ROM = minimal.sfc

.PHONY: all clean

all: $(ROM)
	@echo "Built: $(ROM)"
	@ls -la $(ROM)

# Compile C to pseudo-assembly
main.ps: main.c
	$(CC) -I$(OPENSNES)/lib/include -c $< -o $@

# Combine crt0 + main into single assembly (simpler for minimal test)
combined.asm: crt0.asm main.ps hdr.asm
	@cat crt0.asm > $@
	@echo "" >> $@
	@echo "; === Compiled C code ===" >> $@
	@sed -e '/^\.include/d' \
	     -e '/^\.RAMSECTION.*APPENDTO/,/^\.ENDS/d' \
	     -e '/^\.SECTION.*APPENDTO/,/^\.ENDS/d' \
	     -e '/^\.RAMSECTION "\.bss"/,/^\.ENDS/d' \
	     -e '/^\.SECTION "\.rodata"/,/^\.ENDS/d' \
	     main.ps >> $@

# Assemble combined file
combined.obj: combined.asm hdr.asm
	$(AS) -o $@ $<

# Link
linkfile: combined.obj
	@echo "[objects]" > $@
	@echo "combined.obj" >> $@

$(ROM): linkfile combined.obj
	$(LD) -v -S $< $@

clean:
	rm -f *.obj *.ps *.asm *.sfc *.sym linkfile
	@# Keep source files
	@git checkout -- crt0.asm hdr.asm 2>/dev/null || true
