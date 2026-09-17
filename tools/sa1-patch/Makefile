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
CFLAGS   = -g -Wall -Wextra -O2 -pedantic -D__BUILD_DATE="\"$(DATESTRING)\"" -D__BUILD_VERSION="\"$(VERSION)\""

ifeq ($(shell uname),Darwin)
	EXT     :=
else ifeq ($(OS),Windows_NT)
	ifneq ($(SANITIZE),1)
	CFLAGS  += -static
	endif
	EXT     :=.exe
else
	ifneq ($(SANITIZE),1)
	CFLAGS  += -static
	endif
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

# SANITIZE=1: ASan + UBSan build for the host-side sanitizer job
# (make test-sanitizers). Not static — the sanitizer runtimes do not link
# statically — and -O1 for readable reports.
ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
endif
