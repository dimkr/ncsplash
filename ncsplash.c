/*
 * ncsplash.c - the main module of ncsplash
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* for I/O stuff */
#include <fcntl.h>
#include <unistd.h>
#include <signal.h> /* for signal handling */
#include <curses.h>
#include "config.h"

/*****************************
 * constants and definitions *
 *****************************/

/* coordinates; an object identified by two integers */
typedef struct {
	int x;
	int y;
} coordinate_t;

/* the text written to the FIFO to signal the process to terminate */
#define EXIT_TEXT "exit"

/* exit codes */
enum exit_codes {
	EXIT_CODE_SUCCESS,
	EXIT_CODE_BAD_USAGE,
	EXIT_CODE_ERROR_SIGHANDLER,
	EXIT_CODE_ERROR_FIFO
};

/* the valid number of command-line arguments */
#define VALID_ARGC (2)

/* the usage message */
#define USAGE_MESSAGE "ncsplash FIFO\n\nA simple ncurses-based splash screen " \
                      "which reads strings from a FIFO and prints them to the" \
                      " screen, one at a time.\n"

/* the output file descriptor for messages */
#define OUTPUT_FD STDOUT_FILENO

/* the I/O buffer size */
#define BUFFER_SIZE (512)

/***********
 * globals *
 ***********/

/* the FIFO file descriptor */
static int fifo = 0;

/* the text position */
static coordinate_t position = {0, 0};

/* the window */
static WINDOW *window = NULL;

/* the process ID */
static pid_t pid = 0;

/* a boolean which indicates whether this is the first string drawn */
bool is_first = TRUE;

/*********************
 * utility functions *
 *********************/

 /* m_print (void)
 * prints a variable number of strings
 * input: strings, terminated by a NULL parameter
 * output: - */
void m_print(const char *str, ...) {
	/* the strings to print */
	va_list strings = {{'\0'}};

	/* the current string */
	const char *string = NULL;

	/* the length of each parameter */
	size_t length = 0;

	/* initialize the list of strings */
	va_start(strings, str);

	/* write all strings to standard output */
	for (string = str; NULL != string; string = va_arg(strings, const char *))
	{
		/* calculate the length of each parameter */
		length = 1 + strnlen(string, BUFFER_SIZE);

		/* output the string; upon failure, break the loop */
		if (length != write(OUTPUT_FD, string, length))
			break;
	}

	/* free the strings list */
	va_end(strings);
}

/*********************
 * drawing functions *
 *********************/

/* draw_text (bool)
 * prints a given string on the screen
 * input: a string to print and its position on the screen
 * output: TRUE on success, otherwise FALSE */
bool draw_text(const coordinate_t *position, const char *text) {
	if (ERR == mvprintw(position->y, position->x, text))
		return FALSE;
	return TRUE;
}

/* init_screen (WINDOW *)
 * initializes the screen and returns its dimensions
 * input: a coordinate_t structure
 * output: a valid WINDOW pointer on success or NULL on failure */
WINDOW *init_screen(coordinate_t *dimensions) {
	/* the return value */
	WINDOW *window = NULL;

	/* initialize the screen */
	if (NULL == (window = initscr())) {
		m_print("Error: failed to initialize the screen.\n", NULL);
		return NULL;
	}

	/* disable echoing */
	if (OK != noecho()) {
		m_print("Error: failed to disable screen echo.\n", NULL);
		return NULL;
	}

	/* make the cursor invisible */
	if (ERR == curs_set(0)) {
		m_print("Error: failed to make the cursor invisible.\n", NULL);
		return NULL;
	}

	/* initscr() makes the first refresh() clear the screen - do this now */
	if (ERR == refresh())
		m_print("Error: failed to clear the screen.\n", NULL);

	/* get the screen dimensions */
	getmaxyx(window, dimensions->y, dimensions->x);

	return window;
}

/******************
 * implementation *
 ******************/

/* clean_up (void)
 * performs clean up operations
 * input: -
 * output: - */
void clean_up() {
	/* close the FIFO */
	if (-1 != fifo)
		(void) close(fifo);

	/* close the screen */
	if (NULL != window)
		(void) endwin();
}

/* signal_handler (void)
 * a signal handler; the FIFO is opened asynchronously, so the process receives
 * SIGIO every time data is written to it - this function is called once this
 * happens and handles the data
 * input: the received signal
 * output: - */
void signal_handler(int signal) {
	/* the reading buffer */
	char buffer[BUFFER_SIZE] = {0};

	/* make sure the right signal was received */
	if (SIGIO != signal)
		return;

	/* read from the FIFO */
	if (-1 == read(fifo, &buffer, BUFFER_SIZE)) {
		m_print("Error: failed to read from the FIFO.\n", NULL);
		return;
	}

	/* if the exit text was received, terminate the process */
	if (0 == strncmp(buffer, EXIT_TEXT, sizeof(EXIT_TEXT) - 1)) {
		clean_up();
		exit(EXIT_CODE_SUCCESS);
	}

	/* clear the screen; if it's the first drawing, this will prevent the
	 * drawing of the first string, so skip it once */
	if (TRUE == is_first) {
		if (ERR == erase()) {
			m_print("Error: failed to clear the screen.\n");
			return;
		}
		is_first = FALSE;
	}

	/* draw the read text */
	if (FALSE == draw_text(&position, buffer)) {
		m_print("Error: failed to draw the text.\n", NULL);
		return;
	}

	/* flush the drawing request */
	if (ERR == refresh())
		m_print("Error: failed to flush the drawing request.\n", NULL);
}

/* main (main)
 * the entry point
 * input: the command-line arguments and their count
 * output: an exit_codes member */
int main(int argc, char *argv[]) {
	/* the return value */
	int ret = EXIT_FAILURE;

	/* the screen dimensions */
	coordinate_t screen_dimensions = {0, 0};

	/* the SIGIO signal action */
	struct sigaction signal_action = {{0}};

	/* the flags the FIFO was opened with */
	int flags = 0;

	/* check the command-line */
	if ((VALID_ARGC != argc) ||
	    ((VALID_ARGC == argc) && ('\0' == *argv[1]))) {
		m_print(USAGE_MESSAGE, NULL);
		ret = EXIT_CODE_BAD_USAGE;
		goto end;
	}

	/* initialize the signal action structure */
	signal_action.sa_handler = signal_handler;

	/* do not drop signals received while the signal handler is called - this
	 * way, some data written to the FIFO is lost */
	signal_action.sa_flags = SA_NODEFER;

	/* register the signal handler */
	if (0 != sigaction(SIGIO, &signal_action, NULL)) {
		m_print("Error: failed to register the signal handler.\n", NULL);
		ret = EXIT_CODE_ERROR_SIGHANDLER;
		goto end;
	}

	/* open the FIFO in asynchronous mode, so the process gets notified of
	 * written data through the SIGIO signal */
	if (-1 == (fifo = open(argv[1], O_RDONLY | O_NONBLOCK))) {
		m_print("Error: failed to open the FIFO.\n", NULL);
		ret = EXIT_CODE_ERROR_FIFO;
		goto end;
	}

	/* get the process ID */
	pid = getpid();

	/* make the current process receive SIGIO whenever data is written to the
	 * FIFO and set the asynchronous I/O flag */
	if ((-1 == fcntl(fifo, F_SETOWN, pid)) ||
	    (-1 == (flags = fcntl(fifo, F_GETFL))) ||
	    (-1 == fcntl(fifo, F_SETFL, flags | O_ASYNC))) {
		m_print("Error: failed to enable asynchronous I/O with the FIFO.\n");
		ret = EXIT_CODE_ERROR_FIFO;
		goto end;
	}

	/* initialize the screen */
	if (NULL == (window = init_screen(&screen_dimensions)))
		goto end;

	/* set the text position */
	position.x = CONFIG_TEXT_X;
	position.y = screen_dimensions.y - CONFIG_TEXT_Y;

	/* run an infinite loop; once EXIT_TEXT is received, the signal handler
	 * terminates the process */
	for (; ;) {};

	/* report success */
	ret = EXIT_CODE_SUCCESS;

end:
	/* clean up */
	clean_up();

	return ret;
}