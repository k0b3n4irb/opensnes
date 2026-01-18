# gfx2snes Makefile
# SNES graphics converter tool for OpenSNES

VERSION := 1.0.0

CC      := gcc
CFLAGS  := -Wall -Wextra -O2 -pedantic
CFLAGS  += -DVERSION=\"$(VERSION)\"

# Use absolute path for includes (fixes MSYS2 issues)
SRCDIR  := $(CURDIR)/src
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

SRCS := $(SRCDIR)/gfx2snes.c $(SRCDIR)/loadbmp.c

$(TARGET): $(SRCS) | $(BUILD)
	@echo "[CC] gfx2snes"
	$(CC) $(CFLAGS) -I$(SRCDIR) $(SRCS) -o $@ -lm

$(BUILD):
	@mkdir -p $(BUILD)

install: $(TARGET)
	@echo "Installing $(TARGET) to ../../bin/"
	@mkdir -p ../../bin
	@cp $(TARGET) ../../bin/

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)
