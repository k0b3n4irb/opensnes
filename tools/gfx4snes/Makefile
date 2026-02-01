# gfx4snes - SNES Graphics Converter
# Based on PVSnesLib gfx4snes by Alekmaul
# https://github.com/alekmaul/pvsneslib

# Version number and build date string
VERSION    := 2.0.0
DATESTRING := $(shell date +%Y%m%d)

# Compiler and compiler flags
CC     := clang
CFLAGS  = -g -Wall -O2 -pedantic -D__BUILD_DATE="\"$(DATESTRING)\"" -D__BUILD_VERSION="\"$(VERSION)\""

# Define the libraries and compilation flags to be used depending on the OS.
ifeq ($(shell uname),Darwin)
	EXT     :=
else ifeq ($(OS),Windows_NT)
    CFLAGS  += -static
	EXT     :=.exe
else
	CFLAGS 	+= -static
	EXT     :=
endif

# Source and object file locations
SRC := src
OBJ := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Executable binary file name
EXE     := gfx4snes

# Define the default target
all: $(EXE)$(EXT)

# Define the recipe for linking the executable
$(EXE)$(EXT): $(OBJS)
	@echo "Linking $@"
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Ensure build directory exists
$(OBJ):
	@mkdir -p $(OBJ)

# Define the recipe for compiling object files
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

# Define the install target (copy to OpenSNES bin/)
install: $(EXE)$(EXT)
	@cp $(EXE)$(EXT) ../../bin/$(EXE)$(EXT)

# Define the clean target
clean:
	rm -rf $(OBJ)
	rm -f $(EXE)$(EXT)

# Declare the phony targets
.PHONY: all clean install
