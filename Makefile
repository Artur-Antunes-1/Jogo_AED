CC = gcc
SRC = src/main.c
OUT = build/meujogo

CFLAGS_PKG  := $(shell pkg-config --cflags sdl3 sdl3-ttf 2>/dev/null)
LIBS_PKG    := $(shell pkg-config --libs   sdl3 sdl3-ttf 2>/dev/null)

ifeq ($(strip $(CFLAGS_PKG)),)
  CFLAGS = -I/opt/homebrew/include
  LIBS   = -L/opt/homebrew/lib -lSDL3 -lSDL3_ttf
else
  CFLAGS = $(CFLAGS_PKG)
  LIBS   = $(LIBS_PKG)
endif

# adiciona o include local (shim)
CFLAGS += -Ilocal_include

all: $(OUT)
$(OUT): $(SRC)
	mkdir -p build
	$(CC) $(SRC) -o $(OUT) $(CFLAGS) $(LIBS)

run: all
	./$(OUT)

clean:
	rm -rf build
