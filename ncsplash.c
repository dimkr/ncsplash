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
#include <signal.h> /* for signal handling stuff */
#include <unistd.h> /* for pause() */
#include <sys/stat.h> /* for I/O stuff */
#include <fcntl.h>
#include <string.h> /* for strncmp() */
#include "drawing.h" /* for the drawing backend */
#include "config.h" /* for the configuration */

/*****************************
 * constants and definitions *
 *****************************/

/* the valid number of command-line arguments */
#define VALID_ARGC (3)

/* the usage message */
#define USAGE_MESSAGE "ncsplash FIFO LOGO\n\nA simple ncurses-based splash " \
                      "screen which reads strings from a FIFO and prints " \
                      "them to the screen, one at a time.\n"

/* the text written to the FIFO to signal the process to terminate */
#define EXIT_TEXT "exit"

/* the I/O buffer size */
#define BUFFER_SIZE (512)

/* string length */
#define STRLEN(x) (sizeof(x) - 1)

/* the maximum length of the logo text */
#define MAX_LOGO_LENGTH (128)

/******************
 * implementation *
 ******************/

/* the singal received */
int received_signal = 0;

void signal_handler(int sig) {
	/* save the received signal */
	received_signal = sig;
}

int main(int argc, char *argv[]) {
	/* the return value */
	int ret = EXIT_FAILURE;

	/* the FIFO file descriptor */
	int fifo = 0;

	/* the flags the FIFO was opened with */
	int flags = 0;

	/* the reading buffer */
	char buffer[BUFFER_SIZE] = {0};

	/* the number of bytes read from the FIFO */
	ssize_t read_bytes = 0;

	/* the signal action */
	struct sigaction signal_action = {{0}};

	/* the process ID */
	static pid_t pid = 0;

	/* the drawing area */
	drawing_t drawing = {0};

	/* the drawing position for the status lines */
	position_t status_position = {CONFIG_TEXT_X, 0};

	/* the drawing position for the logo text */
	position_t logo_position = {0, 0};

	/* a boolean which indicates whether this is the first read */
	bool is_first = TRUE;

	/* if an invalid number of command-line arguments was passed,  show the
	 * usage message and exit */
	if (VALID_ARGC != argc) {
		(void) write(STDOUT_FILENO, USAGE_MESSAGE, sizeof(USAGE_MESSAGE));
		goto end;
	}

	/* open the FIFO in non-blocking mode */
	if (-1 == (fifo = open(argv[1], O_RDONLY | O_NONBLOCK)))
		goto end;

	/* initialize the drawing area */
	if (FALSE == drawing_new(&drawing))
		goto end;

	/* initialize the signal action structure */
	signal_action.sa_handler = signal_handler;

	/* do not drop signals received while the signal handler is called - this
	 * way, some data written to the FIFO is lost */
	signal_action.sa_flags = SA_NODEFER;

	/* register the signal handler */
	if (0 != sigaction(SIGIO, &signal_action, NULL))
		goto end;

	/* get the process ID */
	pid = getpid();

	/* make the current process receive SIGIO whenever data is written to the
	 * FIFO and set the asynchronous I/O flag */
	if ((-1 == fcntl(fifo, F_SETOWN, pid)) ||
	    (-1 == (flags = fcntl(fifo, F_GETFL))) ||
	    (-1 == fcntl(fifo, F_SETFL, flags | O_ASYNC)))
		goto end;

	/* draw the logo */
	if ('\0' != *argv[2]) {
		/* calculate the logo text position */
		logo_position.x = (drawing.dimensions.width -
						   strnlen(argv[2], MAX_LOGO_LENGTH)) / 2;
		logo_position.y = drawing.dimensions.height / 2;


		if (FALSE == drawing_draw_text(&drawing, argv[2], &logo_position))
			goto end;

		/* flush the drawing request */
		if (FALSE == drawing_refresh(&drawing))
			goto end;
	}

	/* set the status text position */
	status_position.y = drawing.dimensions.height - CONFIG_TEXT_Y;

	/* enter a signal handling loop */
	do {
		if (FALSE == is_first) {
			/* wait for a signal to be received and handled by the signal
			 * handler */
			(void) pause();

			/* if the received event is not SIGIO, ignore it */
			if (SIGIO != received_signal)
				continue;
		}

		/* perform one initial read before waiting for signals */
		is_first = FALSE;

		/* read from the FIFO */
		if (-1 == (read_bytes = read(fifo, &buffer, (BUFFER_SIZE - 1))))
			break;

		/* if nothing was read from the FIFO, try again */
		if (0 == read_bytes)
			continue;

		/* terminate the read string */
		buffer[read_bytes] = '\0';

		/* if the exit text was received, break the loop and report success */
		if (read_bytes == STRLEN(EXIT_TEXT)) {
			if (0 == strncmp(buffer, EXIT_TEXT, STRLEN(EXIT_TEXT))) {
				ret = EXIT_SUCCESS;
				break;
			}
		}

		/* clear the drawing */
		if (FALSE == drawing_clear(&drawing))
			break;

		/* draw the read text */
		if (FALSE == drawing_draw_text(&drawing, buffer, &status_position))
			break;

		/* draw the logo */
		if ('\0' != *argv[2]) {
			if (FALSE == drawing_set_underlined(&drawing, TRUE))
				goto end;

			if (FALSE == drawing_draw_text(&drawing, argv[2], &logo_position))
				break;

			if (FALSE == drawing_set_underlined(&drawing, FALSE))
				goto end;
		}

		/* flush all drawing requests */
		if (FALSE == drawing_refresh(&drawing))
			break;

	} while (TRUE);

end:
	/* destroy the drawing area */
	(void) drawing_destroy(&drawing);

	return ret;
}
