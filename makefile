CC = gcc

LIBS = -lncurses -lcdk

all:
	$(CC) $(LIBS) -o converse converse.c
