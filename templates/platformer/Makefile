#==============================================================================
# OpenSNES Template - Platformer
#==============================================================================
#
# A simple platformer game template demonstrating:
# - Player movement and jumping
# - Collision detection
# - Scrolling backgrounds
# - Basic enemy AI
#
# Build: make
# Clean: make clean
# Run:   mesen platformer.sfc
#
#==============================================================================

# Project name (output ROM name)
PROJECT = platformer

# OpenSNES SDK location
OPENSNES_HOME ?= ../..

#------------------------------------------------------------------------------
# Tools
#------------------------------------------------------------------------------

CC = $(OPENSNES_HOME)/bin/816-tcc
AS = wla-65816
LD = wlalink
GFX = $(OPENSNES_HOME)/bin/gfx4snes

#------------------------------------------------------------------------------
# Directories
#------------------------------------------------------------------------------

SRCDIR = src
DATADIR = data
BUILDDIR = build

#------------------------------------------------------------------------------
# Files
#------------------------------------------------------------------------------

# Source files
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
ASM_SOURCES = $(wildcard $(SRCDIR)/*.asm)

# Data files (will be generated from assets)
DATA_FILES = $(wildcard $(DATADIR)/*.asm)

# Object files
C_OBJECTS = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.obj)
ASM_OBJECTS = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR)/%.obj)
DATA_OBJECTS = $(DATA_FILES:$(DATADIR)/%.asm=$(BUILDDIR)/%.obj)

ALL_OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS) $(DATA_OBJECTS)

#------------------------------------------------------------------------------
# Flags
#------------------------------------------------------------------------------

CFLAGS = -I$(OPENSNES_HOME)/lib/include
ASFLAGS = -o
LDFLAGS =

#------------------------------------------------------------------------------
# Output
#------------------------------------------------------------------------------

ROM = $(PROJECT).sfc
SYM = $(PROJECT).sym

#------------------------------------------------------------------------------
# Targets
#------------------------------------------------------------------------------

.PHONY: all clean run

all: $(ROM)
	@echo ""
	@echo "Build complete: $(ROM)"
	@echo "Run with: mesen $(ROM)"

$(ROM): $(ALL_OBJECTS) linkfile
	@echo "[LD] Linking $@"
	$(LD) -v -S $(SYM) linkfile $@

# C source compilation
$(BUILDDIR)/%.obj: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $(BUILDDIR)/$*.ps
	# TODO: Run through 816-opt and constify
	$(AS) $(ASFLAGS) $@ $(BUILDDIR)/$*.asm

# Assembly source
$(BUILDDIR)/%.obj: $(SRCDIR)/%.asm | $(BUILDDIR)
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) $@ $<

# Data files
$(BUILDDIR)/%.obj: $(DATADIR)/%.asm | $(BUILDDIR)
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) $@ $<

# Linker configuration
linkfile: | $(BUILDDIR)
	@echo "[GEN] linkfile"
	@echo "[objects]" > linkfile
	@for obj in $(ALL_OBJECTS); do echo "$$obj" >> linkfile; done

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

clean:
	@echo "[CLEAN] Removing build files..."
	-rm -rf $(BUILDDIR)
	-rm -f $(ROM) $(SYM) linkfile
	-rm -f $(SRCDIR)/*.ps $(SRCDIR)/*.asm

run: $(ROM)
	@echo "[RUN] Starting emulator..."
	mesen $(ROM)

#------------------------------------------------------------------------------
# Asset conversion (examples)
#------------------------------------------------------------------------------

# Convert PNG to SNES tiles
# $(DATADIR)/%.asm: assets/%.png
# 	$(GFX) -t -p -o $@ $<
