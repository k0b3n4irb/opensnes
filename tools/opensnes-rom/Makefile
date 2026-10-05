# opensnes-rom — the post-link checks of a user build (and later the header, checksum, region)
TOOL        := opensnes-rom
VERSION     := 1.0.0
SRCS        := $(wildcard src/*.c) ../common/cli.c
HDRS        := $(wildcard src/*.h) ../common/cli.h
INCLUDES    := -Isrc -I../common
TOOL_CFLAGS := -std=c11 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
include ../tool.mk
