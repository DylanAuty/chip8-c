CC=gcc
CFLAGS=-lncurses

all: chip8

chip8: main.c
	$(CC) -o chip8 main.c $(CFLAGS)

clean:
	rm ./chip8
