#==============================================================================
# OpenSNES Library - Makefile
#==============================================================================
#
# Builds the OpenSNES library for both LoROM and HiROM modes.
# Output:
#   build/lorom/*.o  - Object files for LoROM projects
#   build/hirom/*.o  - Object files for HiROM projects
#
# Usage:
#   make            # Build library (both modes)
#   make lorom      # Build LoROM library only
#   make hirom      # Build HiROM library only
#   make clean      # Clean all build artifacts
#
#==============================================================================

.PHONY: all lorom hirom clean

# Directories
SRCDIR = source
INCDIR = include
BUILDDIR_LOROM = build/lorom
BUILDDIR_HIROM = build/hirom

# Tools (using QBE-based compiler)
CC = ../bin/cc65816
AS = ../bin/wla-65816

# Flags
CFLAGS = -I$(INCDIR)
ASFLAGS = -I $(SRCDIR) -o

# Source files
# Note: crt0.asm is startup code that requires hdr.asm - each ROM provides its own
# Note: sm_spc.asm is included by snesmod.asm, so we exclude it from direct compilation
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
ASM_SOURCES = $(filter-out $(SRCDIR)/crt0.asm $(SRCDIR)/sm_spc.asm, $(wildcard $(SRCDIR)/*.asm))

# Object files - LoROM
C_OBJECTS_LOROM = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_LOROM)/%.o)
ASM_OBJECTS_LOROM = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_LOROM)/%-asm.o)
OBJECTS_LOROM = $(C_OBJECTS_LOROM) $(ASM_OBJECTS_LOROM)

# Object files - HiROM
C_OBJECTS_HIROM = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_HIROM)/%.o)
ASM_OBJECTS_HIROM = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_HIROM)/%-asm.o)
OBJECTS_HIROM = $(C_OBJECTS_HIROM) $(ASM_OBJECTS_HIROM)

#------------------------------------------------------------------------------
# Main Target - Build Both
#------------------------------------------------------------------------------

all: lorom hirom
	@echo "========================================"
	@echo "  Library built:"
	@echo "    LoROM: $(BUILDDIR_LOROM)/"
	@echo "    HiROM: $(BUILDDIR_HIROM)/"
	@echo "========================================"

#------------------------------------------------------------------------------
# LoROM Build
#------------------------------------------------------------------------------

lorom: $(OBJECTS_LOROM)
	@echo "[DONE] LoROM library: $(BUILDDIR_LOROM)/"

# C compilation for LoROM
#
# The sed transformations convert cproc/QBE output to WLA-DX compatible assembly:
#   - /^\.data/d         : Remove .data section markers (WLA-DX uses different syntax)
#   - /^\/\* end/d       : Remove "/* end function */" comments from QBE output
#   - /^\.balign/d       : Remove alignment directives (handled by WLA-DX sections)
#   - /^\.GLOBAL/d       : Remove global declarations (WLA-DX exports differently)
#   - .byte -> .db       : Convert byte directive to WLA-DX syntax
#   - .word -> .dw       : Convert word directive to WLA-DX syntax
#   - .long -> .dl       : Convert long directive to WLA-DX syntax
#   - .ascii -> .ASC     : Convert ASCII string directive to WLA-DX syntax
#   - \000 -> ", 0, "    : Convert null bytes in strings to explicit zeros
#   - .dsb N -> .dsb N,0 : Add fill value to DSB (Define Storage Bytes) directive
#
$(BUILDDIR_LOROM)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_LOROM)
	@echo "[CC-LOROM] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_LOROM)/$*.c.asm
	@if [ ! -f $(SRCDIR)/lib_memmap.inc ]; then \
		echo "ERROR: $(SRCDIR)/lib_memmap.inc not found"; exit 1; \
	fi
	@echo '.include "lib_memmap.inc"' > $(BUILDDIR_LOROM)/$*.asm
	@echo '' >> $(BUILDDIR_LOROM)/$*.asm
	@sed -e '/^\.data/d' \
	     -e '/^\/\* end/d' \
	     -e '/^\.balign/d' \
	     -e '/^\.GLOBAL/d' \
	     -e 's/\.byte/.db/g' \
	     -e 's/\.word/.dw/g' \
	     -e 's/\.long/.dl/g' \
	     -e 's/\.ascii/.ASC/g' \
	     -e 's/\\000/", 0, "/g' \
	     -e 's/\.dsb \([0-9]*\)$$/.dsb \1, 0/g' \
	     $(BUILDDIR_LOROM)/$*.c.asm >> $(BUILDDIR_LOROM)/$*.asm
	@$(AS) $(ASFLAGS) $@ $(BUILDDIR_LOROM)/$*.asm

# ASM compilation for LoROM
$(BUILDDIR_LOROM)/%-asm.o: $(SRCDIR)/%.asm | $(BUILDDIR_LOROM)
	@echo "[AS-LOROM] $<"
	@$(AS) $(ASFLAGS) $@ $<

$(BUILDDIR_LOROM):
	@mkdir -p $(BUILDDIR_LOROM)

#------------------------------------------------------------------------------
# HiROM Build
#------------------------------------------------------------------------------

hirom: $(OBJECTS_HIROM)
	@echo "[DONE] HiROM library: $(BUILDDIR_HIROM)/"

# C compilation for HiROM
# (See LoROM rule above for sed transformation documentation)
$(BUILDDIR_HIROM)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_HIROM)
	@echo "[CC-HIROM] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_HIROM)/$*.c.asm
	@if [ ! -f $(SRCDIR)/lib_memmap_hirom.inc ]; then \
		echo "ERROR: $(SRCDIR)/lib_memmap_hirom.inc not found"; exit 1; \
	fi
	@echo '.include "lib_memmap_hirom.inc"' > $(BUILDDIR_HIROM)/$*.asm
	@echo '' >> $(BUILDDIR_HIROM)/$*.asm
	@sed -e '/^\.data/d' \
	     -e '/^\/\* end/d' \
	     -e '/^\.balign/d' \
	     -e '/^\.GLOBAL/d' \
	     -e 's/\.byte/.db/g' \
	     -e 's/\.word/.dw/g' \
	     -e 's/\.long/.dl/g' \
	     -e 's/\.ascii/.ASC/g' \
	     -e 's/\\000/", 0, "/g' \
	     -e 's/\.dsb \([0-9]*\)$$/.dsb \1, 0/g' \
	     $(BUILDDIR_HIROM)/$*.c.asm >> $(BUILDDIR_HIROM)/$*.asm
	@$(AS) $(ASFLAGS) $@ $(BUILDDIR_HIROM)/$*.asm

# ASM compilation for HiROM
$(BUILDDIR_HIROM)/%-asm.o: $(SRCDIR)/%.asm | $(BUILDDIR_HIROM)
	@echo "[AS-HIROM] $<"
	@$(AS) -D HIROM $(ASFLAGS) $@ $<

$(BUILDDIR_HIROM):
	@mkdir -p $(BUILDDIR_HIROM)

#------------------------------------------------------------------------------
# Clean
#------------------------------------------------------------------------------

clean:
	@echo "[CLEAN] Removing build files..."
	-rm -rf build
