# opensnes-level — Tiled levels to SNES map data (the 1.x successor of
# tmx2snes; its converter, tools/tmx2snes/src/level.c, is linked here).
TOOL        := opensnes-level
VERSION     := 1.0.0
TMX         := ../tmx2snes/src
SRCS        := $(wildcard src/*.c) ../common/cli.c ../common/incfile.c $(TMX)/level.c
HDRS        := $(wildcard src/*.h) ../common/cli.h ../common/incfile.h $(TMX)/level.h ../third_party/cute_tiled.h
INCLUDES    := -Isrc -I../common -I$(TMX) -I../third_party
LDLIBS      := -lm
include ../tool.mk
