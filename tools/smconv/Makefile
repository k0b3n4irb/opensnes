#==============================================================================
# smconv - SNESMOD Soundbank Converter
#==============================================================================
#
# This tool converts Impulse Tracker (.it) modules into SNESMOD soundbanks.
# It generates both assembly data and a C header with module/effect IDs.
#
# Usage:
#   smconv -s -o soundbank music.it effects.it
#
# Output:
#   - soundbank.asm - Assembly data for WLA-DX
#   - soundbank.h   - Module/effect ID constants
#
# Based on smconv from PVSnesLib (C++ version)
#
#==============================================================================

# Output directory
BINDIR := ../../bin
TARGET := $(BINDIR)/smconv

# Compiler and flags
CXX      := g++
VERSION  := 2.1.0
CXXFLAGS := -O2 -Wall -Wno-unused-result -fpermissive \
            -D__BUILD_DATE="\"$(shell date +%Y%m%d)\"" \
            -D__BUILD_VERSION="\"$(VERSION)\"" \
            -fno-rtti -fno-exceptions

# Platform-specific flags
UNAME := $(shell uname -s)
ifneq ($(UNAME), Darwin)
    CXXFLAGS += -static
endif

# Source files
SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	@echo "[LD] smconv"
	@$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	@echo "[CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BINDIR):
	@mkdir -p $@

install: $(TARGET)
	@echo "[INSTALL] smconv -> $(BINDIR)/"

clean:
	@echo "[CLEAN] Removing smconv build files..."
	@rm -f $(OBJECTS) $(TARGET)
