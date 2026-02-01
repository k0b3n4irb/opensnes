# font2snes Makefile
# SNES font converter tool for OpenSNES

VERSION := 1.0.0

CC      := clang
CFLAGS  := -Wall -Wextra -O2 -pedantic
CFLAGS  += -DVERSION=\"$(VERSION)\"

SRC     := src
BUILD   := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SOURCES))

# Output binary name
TARGET  := font2snes

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - dynamic linking
else ifeq ($(OS),Windows_NT)
    # Windows - static linking
    CFLAGS += -static
    TARGET := font2snes.exe
else
    # Linux - static linking
    CFLAGS += -static
endif

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@ -lm

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

install: $(TARGET)
	@echo "Installing $(TARGET) to ../../bin/"
	@mkdir -p ../../bin
	@cp $(TARGET) ../../bin/

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean all
