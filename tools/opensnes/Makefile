# opensnes — the project tool: init, build, clean, run, test, doctor, upgrade
# (a shell script, scripts/opensnes, until 2026-10-06)
TOOL        := opensnes
VERSION     := 1.0.0
SRCS        := $(wildcard src/*.c) ../common/cli.c
HDRS        := $(wildcard src/*.h) ../common/cli.h
INCLUDES    := -Isrc -I../common
# paths are composed into fixed buffers; one that long is simply not found
TOOL_CFLAGS := -Wno-format-truncation
include ../tool.mk
