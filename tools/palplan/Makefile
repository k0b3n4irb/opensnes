# palplan Makefile
# SNES project shared-palette planner for OpenSNES

VERSION := 1.0.0

CC      := clang
CFLAGS  := -g -Wall -Wextra -O2 -pedantic
CFLAGS  += -DVERSION=\"$(VERSION)\"

SRC     := src
BUILD   := build
SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c,$(BUILD)/%.o,$(SOURCES))

# Output binary name
TARGET  := palplan

# Platform-specific settings
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - dynamic linking
else ifeq ($(OS),Windows_NT)
    # Windows - static linking
    ifneq ($(SANITIZE),1)
    CFLAGS += -static
    endif
    TARGET := palplan.exe
else
    # Linux - static linking
    ifneq ($(SANITIZE),1)
    CFLAGS += -static
    endif
endif

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(OBJECTS) -o $@

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

# SANITIZE=1: ASan + UBSan build for the host-side sanitizer job
# (make test-sanitizers). Not static — the sanitizer runtimes do not link
# statically — and -O1 for readable reports.
ifeq ($(SANITIZE),1)
CFLAGS += -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
endif
