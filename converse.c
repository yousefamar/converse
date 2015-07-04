#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

/** The maximum length (in characters) of any message */
#define LINE_BUFF_LEN 65536
/** How many logged messages are stored in memory */
#define LOG_LEN       1024
#define NEWLINE       '\n'

/**
 * A doubly linked list that stores logged messages.
 */
typedef struct _MsgNode {
	int length;
	char *message;
	struct _MsgNode *previous;
	struct _MsgNode *next;
} MsgNode;

/** The last node in the list (newest logged message) */
MsgNode *newest = NULL;
/** The first node in the list (oldest logged message) */
MsgNode *oldest = NULL;
/** The number of messages currently in the log */
int log_counter = 0;
/** The user input buffer */
char input[LINE_BUFF_LEN];
int input_len = 0;
/** A mutex to make rendering thread-safe */
pthread_mutex_t render_mutex;

/**
 * Adds a message to the bottom of the message log.
 *
 * @param message The message to be logged.
 */
void pushMessage(char *message) {
	/* Create new message node */
	MsgNode *node = (MsgNode*) malloc(sizeof(MsgNode));

	node->length   = strlen(message);
	node->message  = (char*) malloc((node->length + 1) * (sizeof(char)));
	strcpy(node->message, message);
	node->previous = newest;
	node->next     = NULL;

	/* Update references to newest and oldest */
	if (newest != NULL)
		newest->next = node;
	newest = node;
	if (oldest == NULL)
		oldest = node;

	/* Delete oldest message nodes while there are more than LOG_LEN logged nodes */
	for (++log_counter; log_counter > LOG_LEN; --log_counter) {
		node = oldest->next;
		node->previous   = NULL;
		free(oldest->message);
		oldest->previous = NULL;
		oldest->next     = NULL;
		free(oldest);
		oldest = node;
	}
}

/**
 * Redraws the ncurses UI.
 */
void render() {
	int i, id, col, line, lines;
	MsgNode *current = newest;

	pthread_mutex_lock(&render_mutex);

	clear();

	/* Render message log from newest to top of window with rudimentary soft wrapping */
	for (i = 0; i < LINES - 2 && current != NULL; current = current->previous) {
		// TODO: Implement word soft wrapping
		lines = current->length / COLS + 1;
		i += lines;
		id = 0;
		for (line = 0; line < lines; ++line)
			for (col = 0; id < current->length && col < COLS; ++col, ++id)
				if (LINES - 2 - i + line >= 0)
					mvaddch(LINES - 2 - i + line, col, current->message[id]);
	}

	/* Render horizontal rule above user input area */
	move(LINES - 2, 0);
	hline(0, COLS);
	move(LINES - 1, 0);

	/* Render input buffer making sure the rightmost characters are always visible
	 * (if the text is longer than the terminal window allows) */
	id = input_len - COLS;
	if (id < 0) id = 0;
	for (i = 0; i < input_len && i < COLS; ++i)
		mvaddch(LINES - 1, i, input[id++]);

	/* ncurses redraw */
	refresh();

	pthread_mutex_unlock(&render_mutex);
}

/**
 * A thread start routine that watches a file
 * and writes any new lines to the message log.
 *
 * @param arg The path of the file to be watched.
 */
void* watchFile(void* arg) {
	char *path = (char*) arg;
	int i = 0;
	FILE *file;
	char c, c_prev;

	char line_buff[LINE_BUFF_LEN];

	file = fopen(path, "r");

	if (file == NULL) {
		endwin();
		fprintf(stderr, "Could not open input file \"%s\" for reading.\n", path);
		exit(1);
	}

	/* Read characters from the file forever
	 * NOTE: This is to allow for any new data to come in even after the end of
	 *       the file (EOF) which is especially necessary for FIFO files. */
	while (1) {
		c = fgetc(file);

		/* If the end of the file is reached, reduce character polling frequency */
		if(c == EOF) {
			/* Render only on change to EOF to load large files quicker on start */
			if (c_prev != EOF)
				render();
			c_prev = c;
			/* Pause thread for 100ms */
			// TODO: Perhaps count the number of EOFs and scale sleep time relatively.
			usleep(100000);
			continue;
		}

		/* If a newline character is reached, log the line and reset the line buffer */
		if (c == NEWLINE) {
			line_buff[i] = '\0';
			pushMessage(line_buff);
			i = 0;
			c_prev = c;
			continue;
		}

		/* Add any other polled character to the line buffer */
		if (i < (LINE_BUFF_LEN - 1))
			line_buff[i] = c;

		if (i == (LINE_BUFF_LEN - 1))
			fprintf(stderr, "Warning: Line buffer overflow; message truncated.\n");

		c_prev = c;
		++i;
	}

	fclose(file);

	return NULL;
}

/**
 * Handles window resize signal.
 */
void onWinch (int sig) {
	endwin();
	refresh();

	noecho();
	raw();
	keypad(stdscr, TRUE);

	/* Redraw UI */
	render();
}

/**
 * converse entry point.
 *
 * @arg input_file  The path to a file watched for input to the message log.
 * @arg output_file The path to a file to which user input messages are appended.
 */
int main(int argc, char *argv[]) {
	int i, j, ch;
	struct sigaction sa;
	pthread_t thread;
	FILE *file_out;

	/* Print usage and quit if incorrect argument count */
	if (argc != 3) {
		printf("usage: %s input_file output_file\n", argv[0]);
		return 0;
	}

	file_out = fopen(argv[2], "a");

	if (file_out == NULL) {
		fprintf(stderr, "Could not open output file \"%s\" for appending.\n", argv[2]);
		return 1;
	}

	/* Init input buffer to all null chars for faster wrap checks */
	for (i = 0; i < LINE_BUFF_LEN; ++i)
		input[i] = '\0';

	/* Init ncurses */
	initscr();
	noecho();
	raw();
	keypad(stdscr, TRUE);

	/* Register window resize callback */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = onWinch;
	sigaction(SIGWINCH, &sa, NULL);

	render();

	/* Run input_file watch thread */
	pthread_create(&thread, NULL, watchFile, argv[1]);

	/* Read and handle user keyboard input */
	input_len = 0;
	/* Quit on Esc */
	while ((ch = wgetch(stdscr)) != 27) {
		/* Add alphanumeric characters to input buffer */
		if (ch > 31 && ch < 127) {
			if (input_len < (LINE_BUFF_LEN - 1)) {
				input[input_len++] = ch;
			} else {
				fprintf(stderr, "Warning: Input buffer overflow; message truncated.\n");
			}
		} else {
			switch (ch) {
				/* Delete last character in input buffer on backspace */
				case KEY_BACKSPACE:
				case 127:
					if (input_len > 0)
						input[--input_len] = '\0';
					break;
				/* Send message to output_file,
				 * and clear input buffer on Enter */
				case KEY_ENTER:
				case '\n':
				case '\r':
					fprintf(file_out, "%s\n", input);
					fflush(file_out);
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

	/* Clean up and destroy resources */
	fclose(file_out);
	// TODO: Free all memory, destroy threads, and free mutexes

	return 0;
}
