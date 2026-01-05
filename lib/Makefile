#==============================================================================
# OpenSNES Library - Makefile
#==============================================================================

.PHONY: all clean

# Directories
SRCDIR = source
INCDIR = include
BUILDDIR = build
LIBDIR = ../bin

# Output
LIBRARY = $(LIBDIR)/libopensnes.a

# Tools (using new QBE-based compiler)
CC = ../bin/cc65816
AS = ../bin/wla-65816
AR = ar

# Flags
CFLAGS = -I$(INCDIR)
ASFLAGS = -o

# Source files
# Note: crt0.asm is startup code that requires hdr.asm - each ROM provides its own
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
ASM_SOURCES = $(filter-out $(SRCDIR)/crt0.asm, $(wildcard $(SRCDIR)/*.asm))

# Object files
C_OBJECTS = $(C_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
ASM_OBJECTS = $(ASM_SOURCES:$(SRCDIR)/%.asm=$(BUILDDIR)/%.obj)

OBJECTS = $(C_OBJECTS) $(ASM_OBJECTS)

#------------------------------------------------------------------------------
# Targets
#------------------------------------------------------------------------------

all: $(LIBRARY)
	@echo "[LIB] Library built: $(LIBRARY)"

$(LIBRARY): $(OBJECTS) | $(LIBDIR)
	@echo "[AR] Creating $@"
	$(AR) rcs $@ $(OBJECTS)

# C compilation with new compiler
$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) $< -o $(BUILDDIR)/$*.c.asm
	@sed -e '/^\.data/d' \
	     -e '/^\/\* end/d' \
	     -e '/^\.balign/d' \
	     -e '/^\.GLOBAL/d' \
	     -e 's/\.byte/.db/g' \
	     $(BUILDDIR)/$*.c.asm > $(BUILDDIR)/$*.asm
	$(AS) $(ASFLAGS) $@ $(BUILDDIR)/$*.asm

$(BUILDDIR)/%.obj: $(SRCDIR)/%.asm | $(BUILDDIR)
	@echo "[AS] $<"
	$(AS) $(ASFLAGS) $@ $<

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

$(LIBDIR):
	@mkdir -p $(LIBDIR)

clean:
	@echo "[CLEAN] Removing build files..."
	-rm -rf $(BUILDDIR)
	-rm -f $(LIBRARY)
