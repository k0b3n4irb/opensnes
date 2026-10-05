# opensnes-music — Impulse Tracker → SNESMOD soundbank (the 1.x successor of smconv; same converter)
TOOL        := opensnes-music
VERSION     := 1.0.0
SMC         := ../smconv/src
SRCS        := $(wildcard src/*.c) ../common/cli.c $(SMC)/brr.c $(SMC)/io.c $(SMC)/itloader.c $(SMC)/it2spc.c
HDRS        := $(wildcard src/*.h) ../common/cli.h $(wildcard $(SMC)/*.h)
INCLUDES    := -Isrc -I../common -I$(SMC)
TOOL_CFLAGS := -std=c11
LDLIBS      := -lm
include ../tool.mk
