#==============================================================================
# OpenSNES Examples - Makefile
#==============================================================================

# Parallel builds
UNAME := $(shell uname -s)
ifeq ($(OS),Windows_NT)
    MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
else ifeq ($(UNAME),Darwin)
    MAKEFLAGS += -j$(shell sysctl -n hw.ncpu)
else ifeq ($(UNAME),Linux)
    MAKEFLAGS += -j$(shell nproc)
endif

# Find all example directories with Makefiles
EXAMPLES := $(dir $(wildcard */Makefile) $(wildcard */*/Makefile))

# Create build targets for each example
EXAMPLE_TARGETS := $(EXAMPLES:%=build-%)
CLEAN_TARGETS := $(EXAMPLES:%=clean-%)

.PHONY: all clean $(EXAMPLE_TARGETS) $(CLEAN_TARGETS)

all: $(EXAMPLE_TARGETS)

clean: $(CLEAN_TARGETS)

# Build each example (can run in parallel)
$(EXAMPLE_TARGETS): build-%:
	@$(MAKE) -C $*

# Clean each example (can run in parallel)
$(CLEAN_TARGETS): clean-%:
	@$(MAKE) -C $* clean 2>/dev/null || true
