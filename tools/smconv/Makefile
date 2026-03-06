# smconv - SNESMOD Soundbank Converter (Pure C)
# Based on smconv from PVSnesLib (C++ version)
# (C) 2009 Mukunda Johnson, modifications by Alekmaul 2012-2016

# Version number and build date string
VERSION    := 2.1.0
DATESTRING := $(shell date +%Y%m%d)

# Compiler and compiler flags
CC      := clang
CFLAGS   = -g -Wall -Wextra -O2 -pedantic -std=c11 \
           -D__BUILD_DATE="\"$(DATESTRING)\"" \
           -D__BUILD_VERSION="\"$(VERSION)\""

# Platform-specific flags
ifeq ($(shell uname),Darwin)
	EXT     :=
else ifeq ($(OS),Windows_NT)
	CFLAGS  += -static
	EXT     :=.exe
else
	CFLAGS  += -static
	EXT     :=
endif

# Source and object file locations
SRC := src
OBJ := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Executable binary file name
EXE     := smconv

# Output directory
BINDIR  := ../../bin

.PHONY: all clean install

all: $(EXE)$(EXT)

$(EXE)$(EXT): $(OBJS)
	@echo "[LD] smconv"
	@$(CC) $(CFLAGS) $(OBJS) -lm -o $@

$(OBJ):
	@mkdir -p $(OBJ)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

install: $(EXE)$(EXT) | $(BINDIR)
	@echo "[INSTALL] smconv -> $(BINDIR)/"
	@cp $(EXE)$(EXT) $(BINDIR)/$(EXE)$(EXT)

$(BINDIR):
	@mkdir -p $(BINDIR)

clean:
	@echo "[CLEAN] Removing smconv build files..."
	@rm -rf $(OBJ)
	@rm -f $(EXE)$(EXT)
