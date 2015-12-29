#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>

/**
 * The maximum length (in characters) of any message
 * NB: Does not includes ANSI escape sequences
 */
#define LINE_BUFF_LEN 65536
/** How many logged messages are stored in memory */
#define LOG_LEN       1024
#define NEWLINE       '\n'
#define ESC_CHAR      '\e'

/** A map of ANSI escape codes to ncurses attributes */
int attr_map[] = {
	0,
	A_BOLD,
	A_DIM,
	0,
	A_UNDERLINE,
	A_BLINK,
	0,
	A_REVERSE,
	A_INVIS
};

/**
 * A linked list of format states.
 */
typedef struct _FormatState {
	int index;
	int code;
	struct _FormatState *next;
} FormatState;

/**
 * A doubly linked list that stores logged messages.
 */
typedef struct _MsgNode {
	int length;
	char *message;
	FormatState *format_states;
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
int input_len  = 0;
/** The user input text cursor position */
int cursor_pos = 0;
/**
 * A character offset from which the input buffer is rendered
 * to make sure the cursor is always visible if the text is
 * longer than the terminal window allows
 */
int input_offset = 0;
/** A mutex to make rendering thread-safe */
pthread_mutex_t render_mutex;

/**
 * Adds a message to the bottom of the message log.
 *
 * @param message The message to be logged.
 */
void pushMessage(char *message, FormatState *format_states) {
	/* Create new message node */
	MsgNode *node = (MsgNode*) malloc(sizeof(MsgNode));
	FormatState *format_state = NULL, *format_state_next = NULL;

	node->length   = strlen(message);
	node->message  = (char*) malloc((node->length + 1) * (sizeof(char)));
	strcpy(node->message, message);
	node->format_states = format_states;
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
		node->previous = NULL;
		free(oldest->message);
		/* Delete format states */
		for (format_state = oldest->format_states; format_state != NULL; format_state = format_state_next) {
			format_state_next = format_state->next;
			free(format_state);
		}
		free(oldest);
		oldest = node;
	}
}

/**
 * Redraws the ncurses UI.
 */
void render() {
	int i, id, col, line, lines;
	int curr_color_fg, curr_color_bg;
	MsgNode *current = newest;
	FormatState *format_state = NULL;

	pthread_mutex_lock(&render_mutex);

	clear();

	/* Render message log from newest to top of window with rudimentary soft wrapping */
	for (i = 0; i < LINES - 2 && current != NULL; current = current->previous) {
		format_state = current->format_states;
		/* Reset colors */
		curr_color_fg = -1;
		curr_color_bg = -1;
		/* Reset attributes */
		attroff(A_ATTRIBUTES);
		// TODO: Implement word soft wrapping
		lines = current->length / COLS + 1;
		i += lines;
		id = 0;
		for (line = 0; line < lines; ++line) {
			for (col = 0; id < current->length && col < COLS; ++col, ++id) {
				if (LINES - 2 - i + line >= 0) {

					/* Apply any new formatting up to current char id */
					while (format_state != NULL && id == format_state->index) {
						if (format_state->code < 1) {
							/* Reset attributes */
							attroff(A_ATTRIBUTES);
						} else if (format_state->code < 9) {
							/* Set attribute */
							attron(attr_map[format_state->code]);
						} else if (format_state->code > 20 && format_state->code < 29) {
							/* Reset attribute */
							attroff(attr_map[format_state->code - 20]);

						/* Foreground colors */
						} else if (format_state->code == 39) {
							/* Reset FG color */
							curr_color_fg = -1;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));
						} else if (format_state->code > 29 && format_state->code < 37) {
							/* Set FG color */
							curr_color_fg = format_state->code - 30;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));
						} else if (format_state->code == 97) {
							/* Set FG white */
							curr_color_fg = format_state->code - 90;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));

						/* Background colors */
						} else if (format_state->code == 49) {
							/* Reset BG color */
							curr_color_bg = -1;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));
						} else if (format_state->code > 39 && format_state->code < 47) {
							/* Set BG color */
							curr_color_bg = format_state->code - 40;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));
						} else if (format_state->code == 107) {
							/* Set BG white */
							curr_color_bg = format_state->code - 100;
							attron(COLOR_PAIR((curr_color_fg + 1) + 9 * (curr_color_bg + 1) + 1));

						} else {
							/* Silently ignore unknown escape codes */
						}
						format_state = format_state->next;
					}
					/* Render formatted character */
					mvaddch(LINES - 2 - i + line, col, current->message[id]);
				}
			}
		}
	}

	/* Reset attributes */
	attroff(A_ATTRIBUTES);

	/* Render horizontal rule above user input area */
	move(LINES - 2, 0);
	hline(0, COLS);
	move(LINES - 1, 0);

	/* Make sure the cursor is always visible */
	if (cursor_pos < input_offset)            input_offset = cursor_pos;
	if (cursor_pos > input_offset + COLS - 1) input_offset = cursor_pos - COLS + 1;

	/* Render input buffer
	 * NB: Cannot write to bottom-right corner due to various limitations */
	mvaddnstr(LINES - 1, 0, input + input_offset, COLS - 1);

	/* Move ncurses cursor to correct position */
	move(LINES - 1, cursor_pos - input_offset);

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
	char esc_buf[4];
	int esc_code, esc_count;
	int esc_flag  = FALSE;
	FormatState *format_state      = NULL;
	FormatState *format_state_head = NULL;
	FormatState *format_state_tail = NULL;

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
			// TODO: Perhaps count the number of EOFs and scale sleep time relatively
			usleep(100000);
			continue;
		}

		/* If a newline character is reached, log the line
		 * and reset the line buffer and format state pointers */
		if (c == NEWLINE) {
			line_buff[i] = '\0';
			pushMessage(line_buff, format_state_head);
			i = 0;
			c_prev = c;
			format_state_head = NULL;
			format_state_tail = NULL;
			continue;
		}

		/* Enter ANSI escape code mode */
		if (c == ESC_CHAR) {
			esc_count = 0;
			esc_flag = TRUE;
			continue;
		}

		/* If in ANSI escape code mode, parse ANSI escape codes */
		if (esc_flag) {
			// FIXME: Make left square bracket not optional
			if (c == '[')
				continue;
			/* If the end of an escape code is reached, add it to the format states list */
			if (c == 'm' || c == ';') {
				esc_buf[esc_count] = '\0';
				esc_code = atoi(esc_buf);

				/* Create new format state */
				format_state = (FormatState*) malloc(sizeof(FormatState));
				format_state->index = i;
				format_state->code  = esc_code;
				format_state->next  = NULL;

				/* Add to format states list for current message */
				if (format_state_head == NULL) {
					format_state_head = format_state;
				} else {
					format_state_tail->next = format_state;
				}
				format_state_tail = format_state;

				/* Leave ANSI escape code mode if end of sequence reached */
				if (c == 'm') {
					esc_flag = FALSE;
					continue;
				}
				esc_count = 0;
				continue;
			}
			/* Abort on malformed control sequence (invalid char) */
			if (c < '0' || c > '9') {
				esc_flag = FALSE;
				continue;
			}
			/* Add escape code digit to escape code buffer */
			esc_buf[esc_count++] = c;
			/* Abort on malformed control sequence (too long) */
			if (esc_count > 3) {
				esc_flag = FALSE;
				continue;
			}
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

	/* Init color pairs
	 * Pair IDs are 1-indexed
	 * Color IDs are in the range [-1, 7] where -1 is the terminal default
	 * There are a total of 81 pairs, indexed by ((fg_id + 1) + 9 * (bg_id + 1) + 1) */
	start_color();
	use_default_colors();
	for (i = -1; i < 8; ++i)
		for (j = -1; j < 8; ++j)
			init_pair((i + 1) + 9 * (j + 1) + 1, i, j);

	/* Register window resize callback */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = onWinch;
	sigaction(SIGWINCH, &sa, NULL);

	render();

	/* Run input_file watch thread */
	pthread_create(&thread, NULL, watchFile, argv[1]);

	/* Read and handle user keyboard input */
	input_len  = 0;
	cursor_pos = 0;
	/* Quit on Esc */
	while ((ch = wgetch(stdscr)) != 27) {
		/* Add alphanumeric characters to input buffer at cursor position */
		if (ch > 31 && ch < 127) {
			if (input_len < (LINE_BUFF_LEN - 1)) {
				/* Shift all characters after the cursor forward */
				for (i = input_len; i > cursor_pos; --i)
					input[i] = input[i - 1];
				/* Set typed character */
				input[cursor_pos++] = ch;
				++input_len;
			} else {
				fprintf(stderr, "Warning: Input buffer overflow; message truncated.\n");
			}
		} else {
			switch (ch) {
				/* Delete character before cursor in input buffer on backspace */
				case KEY_BACKSPACE:
				case 127:
					if (cursor_pos < 1)
						break;
					/* Shift all characters after the cursor back */
					for (i = --cursor_pos; i < input_len; ++i)
						input[i] = input[i + 1];
					/* Delete last character */
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
					cursor_pos = 0;
					break;
				/* Move cursor left on pressing left */
				case KEY_LEFT:
					if (cursor_pos > 0)
						--cursor_pos;
					break;
				/* Move cursor right on pressing right */
				case KEY_RIGHT:
					if (cursor_pos < input_len)
						++cursor_pos;
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
