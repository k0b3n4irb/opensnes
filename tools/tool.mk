# tools/tool.mk — the one build recipe for the compiled asset tools.
#
# A tool's Makefile sets a few variables and includes this file:
#
#   TOOL        := gfx4snes          # binary name (required)
#   VERSION     := 2.0.0             # required; the source reads TOOL_VERSION
#                                    # and TOOL_BUILD_DATE (string literals)
#   SRCS        := $(wildcard src/*.c) ../third_party/lodepng.c   # default: src/*.c
#   HDRS        := $(wildcard src/*.h) ../third_party/lodepng.h   # default: src/*.h
#   INCLUDES    := -Isrc -I../third_party                         # default: -Isrc
#   LDLIBS      := -lm
#   TOOL_CFLAGS := -std=c11
#   include ../tool.mk
#
# Targets: all (default), clean, install (copies the binary into ../../bin/),
# debug (-DDEBUG, from clean). Static on Linux and Windows, dynamic on macOS;
# SANITIZE=1 builds with ASan + UBSan at -O1 and dynamic (the sanitizer
# runtimes do not link statically) for `make test-sanitizers`.
#
# Until 2026-10-05 the nine tools each carried a copy of this logic, with
# nine spellings of the same platform block (tools/devtools review, lot 7).

ifndef TOOL
$(error tool.mk: set TOOL before including it)
endif
ifndef VERSION
$(error tool.mk: set VERSION before including it)
endif

CC         ?= clang
DATESTRING := $(shell date +%Y%m%d)
CFLAGS      = -g -Wall -Wextra -O2 -pedantic $(TOOL_CFLAGS) \
              -DTOOL_VERSION=\"$(VERSION)\" -DTOOL_BUILD_DATE=\"$(DATESTRING)\"

ifeq ($(SANITIZE),1)
CFLAGS += -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
else ifneq ($(shell uname -s),Darwin)
CFLAGS += -static
endif

EXT   := $(if $(filter Windows_NT,$(OS)),.exe,)
BIN   := $(TOOL)$(EXT)
BUILD := build

SRCS     ?= $(wildcard src/*.c)
HDRS     ?= $(wildcard src/*.h)
INCLUDES ?= -Isrc
OBJS     := $(addprefix $(BUILD)/,$(addsuffix .o,$(basename $(notdir $(SRCS)))))

all: $(BIN)

$(BIN): $(OBJS)
	@echo "[LD] $@"
	@$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

# One rule per source, by its full path: a vpath over several source
# directories picked ../smconv/src/main.c for wav2brr's main.o.
define TOOL_OBJ_RULE
$(BUILD)/$(basename $(notdir $(1))).o: $(1) $(HDRS) | $(BUILD)
	@echo "[CC] $$<"
	@$$(CC) $$(CFLAGS) $$(INCLUDES) -c $$< -o $$@
endef
$(foreach s,$(SRCS),$(eval $(call TOOL_OBJ_RULE,$(s))))

$(BUILD):
	@mkdir -p $@

install: $(BIN)
	@mkdir -p ../../bin
	@cp $(BIN) ../../bin/

clean:
	@rm -rf $(BUILD) $(BIN)

debug: CFLAGS += -DDEBUG
debug: clean all

.PHONY: all clean install debug
