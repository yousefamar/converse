#include <stdio.h>
#include <string.h>
#include <ncurses.h>

void render() {
	printw("Hello World !!!");

	refresh();
}

int main (int argc, char *argv[]) {
	initscr();

	render();

	getch();
	endwin();

	return 0;
}
