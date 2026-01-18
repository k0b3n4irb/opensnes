# Hello World OpenSNES Test ROM Makefile
# NOTE: This test uses an outdated awk-based build process.
# TODO: Update to use common.mk like the main examples.

.PHONY: all clean
all:
	@echo "[SKIP] hello_world integration test disabled (outdated build)"
clean:
	@echo "[SKIP] Nothing to clean"

# Old build code below - kept for reference
ifdef ENABLE_OLD_BUILD
OPENSNES := $(shell cd ../../.. && pwd)
CC = $(OPENSNES)/bin/cc65816
AS = $(OPENSNES)/bin/wla-65816
LD = $(OPENSNES)/bin/wlalink

ROM = hello_world.sfc

old_all: $(ROM)
	@echo "Built: $(ROM)"
	@ls -la $(ROM)

# Compile C to WLA-DX assembly
main.c.asm: main.c
	$(CC) -I$(OPENSNES)/lib/include $< -o $@

# Combine crt0 + main into single assembly
combined.asm: crt0.asm main.c.asm hdr.asm
	@cat crt0.asm > $@
	@echo "" >> $@
	@echo "; === Compiled C code ===" >> $@
	@awk 'BEGIN{n=0} \
	     /^\.data/{n++; print ".SECTION \".rodata." n "\" SUPERFREE"; next} \
	     /^\/\* end data \*\//{print ".ENDS"; next} \
	     /^\/\* end/{next} \
	     /^\.balign/{next} \
	     /^\.GLOBAL/{next} \
	     {gsub(/\.byte/, ".db"); print}' \
	     main.c.asm >> $@

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
	rm -f *.obj *.c.asm combined.asm *.sfc *.sym linkfile

.gitignore:
	@echo "*.obj" > $@
	@echo "*.c.asm" >> $@
	@echo "*.sfc" >> $@
	@echo "*.sym" >> $@
	@echo "linkfile" >> $@
	@echo "combined.asm" >> $@

endif
