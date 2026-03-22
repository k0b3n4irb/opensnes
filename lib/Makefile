#==============================================================================
# OpenSNES Library - Makefile
#==============================================================================
#
# Builds the OpenSNES library for LoROM, HiROM, and SA-1 modes.
# Output:
#   build/lorom/*.o  - Object files for LoROM projects
#   build/hirom/*.o  - Object files for HiROM projects
#   build/sa1/*.o    - Object files for SA-1 projects
#
# Usage:
#   make            # Build library (all modes)
#   make lorom      # Build LoROM library only
#   make hirom      # Build HiROM library only
#   make sa1        # Build SA-1 library only
#   make clean      # Clean all build artifacts
#
#==============================================================================

.PHONY: all lorom hirom sa1 superfx clean

# Directories
SRCDIR = source
INCDIR = include
BUILDDIR_LOROM = build/lorom
BUILDDIR_HIROM = build/hirom
BUILDDIR_SA1   = build/sa1
BUILDDIR_SUPERFX = build/superfx

# Tools (using QBE-based compiler)
CC = ../bin/cc65816
AS = ../bin/wla-65816

# Flags
CFLAGS = -I$(INCDIR)
ASFLAGS = -I $(SRCDIR) -I $(INCDIR)/asm -I ../templates -o

# Source files
# Note: crt0.asm is startup code that requires hdr.asm - each ROM provides its own
# Note: sm_spc.asm is included by snesmod.asm, so we exclude it from direct compilation
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
ASM_SOURCES = $(filter-out $(SRCDIR)/crt0.asm $(SRCDIR)/sm_spc.asm, $(wildcard $(SRCDIR)/*.asm))

# Sed transformations: convert cproc/QBE output to WLA-DX compatible assembly
#   /^\.data/d         : Remove .data section markers (WLA-DX uses different syntax)
#   /^\/\* end/d       : Remove "/* end function */" comments from QBE output
#   /^\.balign/d       : Remove alignment directives (handled by WLA-DX sections)
#   /^\.GLOBAL/d       : Remove global declarations (WLA-DX exports differently)
#   .byte -> .db       : Convert byte directive to WLA-DX syntax
#   .word -> .dw       : Convert word directive to WLA-DX syntax
#   .long -> .dl       : Convert long directive to WLA-DX syntax
#   .ascii -> .ASC     : Convert ASCII string directive to WLA-DX syntax
#   \000 -> ", 0, "    : Convert null bytes in strings to explicit zeros
#   .dsb N -> .dsb N,0 : Add fill value to DSB (Define Storage Bytes) directive
SED_QBE_TO_WLA = \
	-e '/^\.data/d' \
	-e '/^\/\* end/d' \
	-e '/^\.balign/d' \
	-e '/^\.GLOBAL/d' \
	-e 's/\.byte/.db/g' \
	-e 's/\.word/.dw/g' \
	-e 's/\.long/.dl/g' \
	-e 's/\.ascii/.ASC/g' \
	-e 's/\\000/", 0, "/g' \
	-e 's/\.dsb \([0-9]*\)$$/.dsb \1, 0/g'

# Object files - LoROM
C_OBJECTS_LOROM = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_LOROM)/%.o)
ASM_OBJECTS_LOROM = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_LOROM)/%-asm.o)
OBJECTS_LOROM = $(C_OBJECTS_LOROM) $(ASM_OBJECTS_LOROM)

# Object files - HiROM
C_OBJECTS_HIROM = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_HIROM)/%.o)
ASM_OBJECTS_HIROM = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_HIROM)/%-asm.o)
OBJECTS_HIROM = $(C_OBJECTS_HIROM) $(ASM_OBJECTS_HIROM)

# Object files - SA-1 (same bank layout as LoROM, different header)
C_OBJECTS_SA1 = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_SA1)/%.o)
ASM_OBJECTS_SA1 = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_SA1)/%-asm.o)
OBJECTS_SA1 = $(C_OBJECTS_SA1) $(ASM_OBJECTS_SA1)

# Object files - SuperFX (LoROM-based, same bank layout as standard LoROM)
C_OBJECTS_SUPERFX = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR_SUPERFX)/%.o)
ASM_OBJECTS_SUPERFX = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR_SUPERFX)/%-asm.o)
OBJECTS_SUPERFX = $(C_OBJECTS_SUPERFX) $(ASM_OBJECTS_SUPERFX)

#------------------------------------------------------------------------------
# Main Target - Build All
#------------------------------------------------------------------------------

all: lorom hirom sa1 superfx
	@echo "========================================"
	@echo "  Library built:"
	@echo "    LoROM:    $(BUILDDIR_LOROM)/"
	@echo "    HiROM:    $(BUILDDIR_HIROM)/"
	@echo "    SA-1:     $(BUILDDIR_SA1)/"
	@echo "    SuperFX:  $(BUILDDIR_SUPERFX)/"
	@echo "========================================"

#------------------------------------------------------------------------------
# LoROM Build
#------------------------------------------------------------------------------

lorom: $(OBJECTS_LOROM)
	@echo "[DONE] LoROM library: $(BUILDDIR_LOROM)/"

# C compilation for LoROM
$(BUILDDIR_LOROM)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_LOROM)
	@echo "[CC-LOROM] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_LOROM)/$*.c.asm
	@echo '.include "memmap.inc"' > $(BUILDDIR_LOROM)/$*.asm
	@echo '' >> $(BUILDDIR_LOROM)/$*.asm
	@sed $(SED_QBE_TO_WLA) $(BUILDDIR_LOROM)/$*.c.asm >> $(BUILDDIR_LOROM)/$*.asm
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
$(BUILDDIR_HIROM)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_HIROM)
	@echo "[CC-HIROM] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_HIROM)/$*.c.asm
	@echo '.include "memmap_hirom.inc"' > $(BUILDDIR_HIROM)/$*.asm
	@echo '' >> $(BUILDDIR_HIROM)/$*.asm
	@sed $(SED_QBE_TO_WLA) $(BUILDDIR_HIROM)/$*.c.asm >> $(BUILDDIR_HIROM)/$*.asm
	@$(AS) $(ASFLAGS) $@ $(BUILDDIR_HIROM)/$*.asm

# ASM compilation for HiROM
$(BUILDDIR_HIROM)/%-asm.o: $(SRCDIR)/%.asm | $(BUILDDIR_HIROM)
	@echo "[AS-HIROM] $<"
	@$(AS) -D HIROM $(ASFLAGS) $@ $<

$(BUILDDIR_HIROM):
	@mkdir -p $(BUILDDIR_HIROM)

#------------------------------------------------------------------------------
# SA-1 Build (same bank layout as LoROM, uses memmap_sa1.inc)
#------------------------------------------------------------------------------

sa1: $(OBJECTS_SA1)
	@echo "[DONE] SA-1 library: $(BUILDDIR_SA1)/"

# C compilation for SA-1
$(BUILDDIR_SA1)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_SA1)
	@echo "[CC-SA1] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_SA1)/$*.c.asm
	@echo '.include "memmap_sa1.inc"' > $(BUILDDIR_SA1)/$*.asm
	@echo '' >> $(BUILDDIR_SA1)/$*.asm
	@sed $(SED_QBE_TO_WLA) $(BUILDDIR_SA1)/$*.c.asm >> $(BUILDDIR_SA1)/$*.asm
	@$(AS) $(ASFLAGS) $@ $(BUILDDIR_SA1)/$*.asm

# ASM compilation for SA-1
$(BUILDDIR_SA1)/%-asm.o: $(SRCDIR)/%.asm | $(BUILDDIR_SA1)
	@echo "[AS-SA1] $<"
	@$(AS) -D SA1 $(ASFLAGS) $@ $<

$(BUILDDIR_SA1):
	@mkdir -p $(BUILDDIR_SA1)

#------------------------------------------------------------------------------
# SuperFX Build (LoROM-based, uses memmap.inc — same as standard LoROM)
#------------------------------------------------------------------------------

superfx: $(OBJECTS_SUPERFX)
	@echo "[DONE] SuperFX library: $(BUILDDIR_SUPERFX)/"

# C compilation for SuperFX
$(BUILDDIR_SUPERFX)/%.o: $(SRCDIR)/%.c | $(BUILDDIR_SUPERFX)
	@echo "[CC-SUPERFX] $<"
	@$(CC) $(CFLAGS) $< -o $(BUILDDIR_SUPERFX)/$*.c.asm
	@echo '.include "memmap.inc"' > $(BUILDDIR_SUPERFX)/$*.asm
	@echo '' >> $(BUILDDIR_SUPERFX)/$*.asm
	@sed $(SED_QBE_TO_WLA) $(BUILDDIR_SUPERFX)/$*.c.asm >> $(BUILDDIR_SUPERFX)/$*.asm
	@$(AS) $(ASFLAGS) $@ $(BUILDDIR_SUPERFX)/$*.asm

# ASM compilation for SuperFX
$(BUILDDIR_SUPERFX)/%-asm.o: $(SRCDIR)/%.asm | $(BUILDDIR_SUPERFX)
	@echo "[AS-SUPERFX] $<"
	@$(AS) -D SUPERFX $(ASFLAGS) $@ $<

$(BUILDDIR_SUPERFX):
	@mkdir -p $(BUILDDIR_SUPERFX)

#------------------------------------------------------------------------------
# Clean
#------------------------------------------------------------------------------

clean:
	@echo "[CLEAN] Removing build files..."
	-rm -rf build
