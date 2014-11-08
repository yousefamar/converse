#include <stdio.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>

void render() {
	//printw("Hello World !!!");
	mvprintw(0, 0, "COLS = %d, LINES = %d", COLS, LINES);

	refresh();
}

void on_winch (int sig) {
	int i;

	endwin();
	refresh();
	clear();

	render();
}

int main (int argc, char *argv[]) {
	struct sigaction sa;

	initscr();

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = on_winch;
	sigaction(SIGWINCH, &sa, NULL);

	render();

	while (getch() != 27);

	endwin();

	return 0;
}
