# sa1-patch — post-link SA-1 map-mode patcher
#
# Patches byte $7FD5 of a built .sfc to mark it as SA-1 instead of
# LoROM. Replaces an inline Python one-liner that used to live in
# make/common.mk (audit P2.4 #3).
#
# Build pattern matches the other tools in tools/ (gfx4snes, smconv,
# img2snes, ...) — clang static-link on Linux/Windows, dynamic on
# macOS. `make install` copies the binary into ../../bin/.

VERSION    := 1.0.0
DATESTRING := $(shell date +%Y%m%d)

CC      := clang
CFLAGS   = -g -Wall -O2 -pedantic -D__BUILD_DATE="\"$(DATESTRING)\"" -D__BUILD_VERSION="\"$(VERSION)\""

ifeq ($(shell uname),Darwin)
	EXT     :=
else ifeq ($(OS),Windows_NT)
	CFLAGS  += -static
	EXT     :=.exe
else
	CFLAGS  += -static
	EXT     :=
endif

SRC     := src
OBJ     := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJS    := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))
EXE     := sa1_patch

all: $(EXE)$(EXT)

$(EXE)$(EXT): $(OBJS)
	@echo "Linking $@"
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(OBJ):
	@mkdir -p $(OBJ)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

install: $(EXE)$(EXT)
	@cp $(EXE)$(EXT) ../../bin/$(EXE)$(EXT)

clean:
	rm -rf $(OBJ)
	rm -f $(EXE)$(EXT)

.PHONY: all clean install
