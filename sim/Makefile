cc = gcc
CC = g++

#PACKAGES = /opt/local/
PACKAGES = /mingw64
SDL_CFLAGS = $(shell $(PACKAGES)/bin/sdl2-config --cflags)
SDL_LIBS   = $(shell $(PACKAGES)/bin/sdl2-config --libs)

GBA_CFLAGS  =
GBA_LDFLAGS = -Llib -ltonc

WIN_CFLAGS  =
WIN_LDFLAGS =

CXXFLAGS= $(WIN_CFLAGS)  $(SDL_CFLAGS) -g -fconcepts -fno-exceptions
LDFLAGS = $(WIN_LDFLAGS) $(SDL_LIBS)

all: main

main.o: main.cpp
	$(CC) $(CXXFLAGS) -c $< -o $@

main: main.o
	$(CC) main.o $(LDFLAGS) -o main

clean:
	rm -f *.o main
