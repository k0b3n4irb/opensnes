# wav2brr Makefile
# WAV -> SNES BRR sample converter for OpenSNES.
# Reuses smconv's BRR encoder (../smconv/src/brr.c) so output matches
# soundbank-baked samples byte-for-byte.

VERSION := 1.0.0

CC      := clang
CFLAGS  := -g -Wall -Wextra -O2 -pedantic -std=c11
CFLAGS  += -DVERSION=\"$(VERSION)\"

SRC       := src
BUILD     := build
SMCONV    := ../smconv/src
# Our own sources plus the shared encoder from smconv.
SOURCES   := $(wildcard $(SRC)/*.c) $(SMCONV)/brr.c
OBJECTS   := $(patsubst %.c,$(BUILD)/%.o,$(notdir $(SOURCES)))

TARGET  := wav2brr

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - dynamic linking
else ifeq ($(OS),Windows_NT)
    CFLAGS += -static
    TARGET := wav2brr.exe
else
    CFLAGS += -static
endif

.PHONY: all clean install debug

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@ -lm

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -I$(SRC) -I$(SMCONV) -c $< -o $@

$(BUILD)/%.o: $(SMCONV)/%.c | $(BUILD)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -I$(SRC) -I$(SMCONV) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

install: $(TARGET)
	@echo "Installing $(TARGET) to ../../bin/"
	@mkdir -p ../../bin
	@cp $(TARGET) ../../bin/

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(TARGET)

debug: CFLAGS += -g -DDEBUG
debug: clean all
