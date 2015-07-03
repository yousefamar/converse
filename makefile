CC = gcc
INSTALL_PATH ?= /usr/local

LIBS = -lncurses -lpthread

all:
	$(CC) $(LIBS) -o converse converse.c

install:
	cp converse $(INSTALL_PATH)/bin

clean:
	rm -rf converse

.PHONY: all install clean
