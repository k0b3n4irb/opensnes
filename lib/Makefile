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

# Tools (will use 816-tcc when ready)
CC = ../bin/816-tcc
AS = wla-65816
AR = ar

# Flags
CFLAGS = -I$(INCDIR)
ASFLAGS = -o

# Source files
C_SOURCES = $(wildcard $(SRCDIR)/*.c)
ASM_SOURCES = $(wildcard $(SRCDIR)/*.asm)

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

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

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
