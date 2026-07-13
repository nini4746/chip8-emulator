# CHIP-8 emulator - macOS + clang + brew SDL2 (pkg-config).
CC      ?= clang
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

# Timer verification log: build with `make CFLAGS_EXTRA=-DCHIP8_TIMER_LOG`.
CFLAGS_EXTRA ?=

TARGET := chip8
SRCS   := main.c chip8.c platform.c
OBJS   := $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(OBJS) -o $@ $(SDL_LIBS) -lm

%.o: %.c
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(SDL_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
