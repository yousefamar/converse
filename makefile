CC = gcc

LIBS = -lncurses

all:
	$(CC) $(LIBS) -o converse converse.c
