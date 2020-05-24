CC=gcc
CFLAGS=-lncurses

all: main

main: main.c
	$(CC) -o main main.c $(CFLAGS)

clean:
	rm ./main
