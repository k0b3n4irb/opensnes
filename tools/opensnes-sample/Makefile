# opensnes-sample — WAV → BRR samples (the 1.x successor of wav2brr; same encoder, smconv's brr.c)
TOOL        := opensnes-sample
VERSION     := 1.0.0
SRCS        := $(wildcard src/*.c) ../common/cli.c ../smconv/src/brr.c
HDRS        := $(wildcard src/*.h) ../common/cli.h $(wildcard ../smconv/src/*.h)
INCLUDES    := -Isrc -I../common -I../smconv/src
TOOL_CFLAGS := -std=c11
LDLIBS      := -lm
include ../tool.mk
