#==============================================================================
# OpenSNES Tests - Makefile
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

# Find all test directories with Makefiles
# Note: $(dir ...) produces trailing slashes; we strip them for cleaner target names
TESTS_RAW := $(dir $(wildcard */Makefile) $(wildcard */*/Makefile))
TESTS := $(patsubst %/,%,$(TESTS_RAW))

# Create build targets for each test
TEST_TARGETS := $(TESTS:%=build-%)
CLEAN_TARGETS := $(TESTS:%=clean-%)

.PHONY: all clean $(TEST_TARGETS) $(CLEAN_TARGETS)

all: $(TEST_TARGETS)

clean: $(CLEAN_TARGETS)

# Build each test (can run in parallel)
$(TEST_TARGETS): build-%:
	@$(MAKE) -C $*

# Clean each test (can run in parallel)
$(CLEAN_TARGETS): clean-%:
	@$(MAKE) -C $* clean 2>/dev/null || true
