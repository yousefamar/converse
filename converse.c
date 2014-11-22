#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>

#define LINE_BUFF_LEN 65536
// TODO: Actually implement this
#define LOG_LEN       1024
#define NEWLINE       '\n'

typedef struct _MsgNode {
	int length;
	char *message;
	struct _MsgNode *previous;
} MsgNode;

MsgNode *latest = NULL;
char input[LINE_BUFF_LEN];
int input_len = 0;

// TODO: Make threadsafe; use mutex
MsgNode* pushMessage(char *message) {
	MsgNode *node = (MsgNode*) malloc(sizeof(MsgNode));

	node->length = strlen(message);
	node->message = (char*) malloc(node->length * (sizeof(char)));
	strcpy(node->message, message);
	node->previous = latest;

	return node;
}

void render() {
	int i, id, col, line, lines;
	MsgNode *current = latest;

	clear();

	if (latest != NULL) {
		for (i = 0; i < LINES - 2 && current != NULL; current = current->previous) {
			// TODO: Implement text wrap
			lines = current->length / COLS + 1;
			i += lines;
			id = 0;
			for (line = 0; line < lines; ++line)
				for (col = 0; id < current->length && col < COLS; ++col, ++id)
					if (LINES - 2 - i + line >= 0)
						mvaddch(LINES - 2 - i + line, col, current->message[id]);
		}
	}

	for (i = 0; i < COLS; i++)
		mvaddch(LINES - 2, i, '_');

	id = input_len - COLS;
	if (id < 0) id = 0;
	for (i = 0; i < input_len && i < COLS; ++i)
		mvaddch(LINES - 1, i, input[id++]);

	refresh();
}

void* watchFile(void* arg) {
	char *path = (char*) arg;
	int i = 0;
	FILE *file;
	char c;

	char line_buff[LINE_BUFF_LEN];

	file = fopen(path, "r");

	if (file == NULL) {
		fprintf(stderr, "Could not read input from %s.\n", path);
		exit(1);
	}

	while ((c = fgetc(file)) != EOF) {
		if (c == NEWLINE) {
			line_buff[i] = '\0';
			latest = pushMessage(line_buff);
			render();
			i = 0;
			continue;
		}

		if (i < (LINE_BUFF_LEN - 1))
			line_buff[i] = c;

		if (i == (LINE_BUFF_LEN - 1))
			fprintf(stderr, "Warning: Line buffer overflow; message truncated.\n");

		++i;
	}

	fclose(file);

	return NULL;
}

void onWinch (int sig) {
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
	pthread_t thread;

	// Init input buffer to all null chars for faster wrap checks
	for (i = 0; i < LINE_BUFF_LEN; ++i)
		input[i] = '\0';

	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = onWinch;
	sigaction(SIGWINCH, &sa, NULL);

	render();

	pthread_create(&thread, NULL, watchFile, "test");

	input_len = 0;
	while ((ch = wgetch(stdscr)) != 27) {
		if (ch > 31 && ch < 127) {
			if (input_len < (LINE_BUFF_LEN - 1)) {
				input[input_len++] = ch;
			} else {
				fprintf(stderr, "Warning: Input buffer overflow; message truncated.\n");
			}
		} else {
			switch (ch) {
				case KEY_ENTER:
				case '\n':
				case '\r':
					latest = pushMessage(input);
					for (j = 0; j < input_len; j++)
						input[j] = '\0';
					input_len = 0;
					break;
				default:
					break;
			}
		}
		render();
	}

	endwin();

	// TODO: Free all memory, destroy threads, and free mutexes

	return 0;
}
