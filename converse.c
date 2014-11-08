#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>

#define INPUT_BUFF_LEN 65536
#define LOG_LEN        1024

typedef struct _MsgNode {
	int length;
	char *message;
	struct _MsgNode *previous;
} MsgNode;

MsgNode *latest = NULL;
char input[INPUT_BUFF_LEN];

MsgNode* pushMessage(char *message) {
	MsgNode *node = (MsgNode*) malloc(sizeof(MsgNode));

	node->length = strlen(message + 1);
	node->message = (char*) malloc(node->length * (sizeof(char)));
	strcpy(node->message, message);
	node->previous = latest;

	return node;
}

void render() {
	int i;
	MsgNode *current = latest;

	clear();

	for (i = 0; i < COLS; i++)
		mvaddch(LINES - 2, i, '_');

	if (latest != NULL) {
		for (i = 0; current != NULL; ++i, current = current->previous) {
			mvprintw(LINES - 3 - i, 0, current->message);
		}
	}

	mvprintw(LINES -1, 0, input);

	refresh();
}

void on_winch (int sig) {
	endwin();
	refresh();

	noecho();
	raw();
	keypad(stdscr, TRUE);

	render();
}

int main(int argc, char *argv[]) {
	int i, j, ch;
	struct sigaction sa;


	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = on_winch;
	sigaction(SIGWINCH, &sa, NULL);

	render();

	i = 0;
	while ((ch = wgetch(stdscr)) != 27) {
		if (ch > 31 && ch < 127) {
			input[i] = ch;
			i++;
		} else {
			switch (ch) {
				case KEY_ENTER:
				case '\n':
				case '\r':
					input[i] = '\0';
					latest = pushMessage(input);
					for (j = 0; j < i; j++)
						input[j] = '\0';
					i = 0;
					break;
				default:
					break;
			}
		}
		render();
	}

	endwin();

	// TODO: Free all memory.

	return 0;
}
