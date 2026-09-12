# img2snes - PNG RGB/RGBA to indexed PNG converter for OpenSNES

# Version number and build date string
VERSION    := 1.0.0
DATESTRING := $(shell date +%Y%m%d)

# Compiler and compiler flags
CC     := clang
CFLAGS  = -g -Wall -O2 -pedantic -D__BUILD_DATE="\"$(DATESTRING)\"" -D__BUILD_VERSION="\"$(VERSION)\""

# Define the libraries and compilation flags to be used depending on the OS.
ifeq ($(shell uname),Darwin)
	EXT     :=
else ifeq ($(OS),Windows_NT)
    ifneq ($(SANITIZE),1)
    CFLAGS  += -static
    endif
	EXT     :=.exe
else
	ifneq ($(SANITIZE),1)
	CFLAGS 	+= -static
	endif
	EXT     :=
endif

# Source and object file locations
SRC := src
OBJ := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

# Executable binary file name
EXE     := img2snes

# Link with math library
LDLIBS  := -lm

# Default target
all: $(EXE)$(EXT)

# Linking
$(EXE)$(EXT): $(OBJS)
	@echo "Linking $@"
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

# Ensure build directory exists
$(OBJ):
	@mkdir -p $(OBJ)

# Compiling
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

# Install to OpenSNES bin/
install: $(EXE)$(EXT)
	@cp $(EXE)$(EXT) ../../bin/$(EXE)$(EXT)

# Clean
clean:
	rm -rf $(OBJ)
	rm -f $(EXE)$(EXT)

.PHONY: all clean install

# SANITIZE=1: ASan + UBSan build for the host-side sanitizer job
# (make test-sanitizers). Not static — the sanitizer runtimes do not link
# statically — and -O1 for readable reports.
ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
endif
