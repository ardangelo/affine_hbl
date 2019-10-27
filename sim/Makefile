cc = gcc
CC = g++

CXXFLAGS=-g -fconcepts -fno-exceptions

SDL_CFLAGS = -I/opt/local/include
SDL_LIBS   = -L/opt/local/lib -lsdl2

all: main

main.o: main.cpp
	$(CC) $(CXXFLAGS) $(SDL_CFLAGS) -c $< -o $@

main: main.o
	$(CC) main.o $(SDL_LIBS) -o main

clean:
	rm -f *.o main
