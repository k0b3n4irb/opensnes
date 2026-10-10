# wav2brr — PCM .wav → .brr sample; the BRR encoder is smconv's brr.c
TOOL        := wav2brr
VERSION     := 1.0.0
SRCS        := $(wildcard src/*.c) ../smconv/src/brr.c
HDRS        := $(wildcard src/*.h) $(wildcard ../smconv/src/*.h)
INCLUDES    := -Isrc -I../smconv/src
TOOL_CFLAGS := -std=c11
LDLIBS      := -lm
include ../tool.mk
