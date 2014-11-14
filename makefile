CC = gcc

LIBS = -lncurses -lpthread

all:
	$(CC) $(LIBS) -o converse converse.c
