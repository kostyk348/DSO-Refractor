CC      = gcc
CXX     = g++
CFLAGS  = -g -O2 -std=c11 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -fPIC
CXXFLAGS= -g -O2 -std=c++17 -Wall -Wextra -fPIC

# Includes
DSR_DIR = /home/lain/dsa
CFLAGS  += -I$(DSR_DIR)/include -D_GNU_SOURCE -D_DEFAULT_SOURCE

# SDL2 via pkg-config
SDL_CFLAGS  := $(shell pkg-config --cflags sdl2)
SDL_LIBS    := $(shell pkg-config --libs sdl2)
CFLAGS      += $(SDL_CFLAGS)

# OpenGL / GLEW
GLEW_LIBS   := $(shell pkg-config --libs glew 2>/dev/null || echo "-lGLEW -lGL")
GL_LIBS     := -lGL

# DSA lib
DSR_LIBS    := -L$(DSR_DIR) -ldsa

# ImGui (for editor, optional)
IMGUI_LIBS  := -limgui

# Sources
DSR_SRCS    := $(wildcard dsr/*.c)
EDITOR_SRCS := $(wildcard editor/*.c)
TOOL_SRCS   := $(wildcard tools/*.c)

# Objects
DSR_OBJS    := $(DSR_SRCS:.c=.o)
EDITOR_OBJS := $(EDITOR_SRCS:.c=.o)
TOOL_OBJS   := $(TOOL_SRCS:.c=.o)

# Targets
.PHONY: all clean lib bin tool test

all: lib bin tool

# Static library
lib: libdsr.a

libdsr.a: $(DSR_OBJS)
	ar rcs $@ $^

# Engine binary (game)
bin: dsr-bin

dsr-bin: main.c libdsr.a
	$(CC) $(CFLAGS) -o $@ main.c libdsr.a $(SDL_LIBS) $(GLEW_LIBS) $(GL_LIBS) $(DSR_LIBS) -lm -ldl 2>&1

# CLI tools
tool: rfatool meshview

rfatool: tools/rfatool.c libdsr.a
	$(CC) $(CFLAGS) -o $@ $< libdsr.a -lm

meshview: tools/meshview.c libdsr.a
	$(CC) $(CFLAGS) -o $@ $< libdsr.a -lm

# Shared library (for hot-reload)
libdsr.so: $(DSR_OBJS)
	$(CC) -shared -o $@ $^ -lm

# Pattern rules
dsr/%.o: dsr/%.c dsr/*.h
	$(CC) $(CFLAGS) -c $< -o $@

editor/%.o: editor/%.c editor/*.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

tools/%.o: tools/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Tests
test: tests/test_rfa.c libdsr.a
	$(CC) $(CFLAGS) -o test_rfa tests/test_rfa.c libdsr.a -lm
	./test_rfa

clean:
	rm -f dsr/*.o editor/*.o tools/*.o
	rm -f dsr-bin libdsr.a libdsr.so rfatool meshview test_rfa
