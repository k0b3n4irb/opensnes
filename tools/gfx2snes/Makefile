# gfx2snes Makefile
# SNES graphics converter tool for OpenSNES

VERSION := 1.0.0

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -pedantic
CFLAGS  += -DVERSION=\"$(VERSION)\"

BUILD   := build
TARGET  := gfx2snes

# Platform-specific
ifeq ($(OS),Windows_NT)
    CFLAGS += -static
    TARGET := gfx2snes.exe
else ifneq ($(shell uname -s),Darwin)
    CFLAGS += -static
endif

.PHONY: all clean install

all: $(TARGET)

# Compile from src directory to avoid MSYS2 path issues with includes
$(TARGET): src/gfx2snes.c src/loadbmp.c | $(BUILD)
	@echo "[CC] gfx2snes"
	@echo "Current dir: $(CURDIR)"
	@ls -la src/
	cd src && pwd && ls -la && $(CC) $(CFLAGS) -I. gfx2snes.c loadbmp.c -o ../$(TARGET) -lm

$(BUILD):
	@mkdir -p $(BUILD)

install: $(TARGET)
	@echo "Installing $(TARGET) to ../../bin/"
	@mkdir -p ../../bin
	@cp $(TARGET) ../../bin/

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)
