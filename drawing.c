/*
 * drawing.c - the ncsplash ncurses-powered drawing backend
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

#include "drawing.h"

bool drawing_new(drawing_t *drawing) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept a NULL pointer */
	if (NULL == drawing)
		goto end;

	/* initialize the window */
	if (NULL == (drawing->window = initscr()))
		goto end;

	/* clear the screen */
	if (ERR == refresh())
		goto end;

	/* disable echoing */
	if (OK != noecho())
		goto end;

	/* make the cursor invisible */
	if (ERR == curs_set(0))
		goto end;

	/* get the window dimensions */
	getmaxyx(drawing->window,
	         drawing->dimensions.height,
	         drawing->dimensions.width);

	/* mark the drawing_t structure as initialized */
	drawing->is_initialized = TRUE;

	/* report success */
	ret = TRUE;

end:
	return ret;
}

bool drawing_destroy(drawing_t *drawing) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept a NULL pointer or uninitialized drawing_t structures */
	if ((NULL == drawing) || (FALSE == drawing->is_initialized))
		goto end;

	/* close the screen */
	if (OK != endwin())
		goto end;

	/* report success */
	ret = TRUE;

end:
	return ret;
}

bool drawing_draw_text(drawing_t *drawing,
                       const char *text,
                       const position_t *position) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept NULL pointers or an empty string */
	if ((NULL == drawing) || \
	    (NULL == text) || \
	    (NULL == position) || \
	    ('\0' == text[0]))
		goto end;

	/* draw the text */
	if (ERR == mvwprintw(drawing->window, position->y, position->x, text))
		goto end;

	/* report success */
	ret = TRUE;

end:
	return ret;
}

bool drawing_clear(drawing_t *drawing, const int y) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept a NULL pointer */
	if (NULL == drawing)
		goto end;

	/* erase the line */
	if (ERR == werase(drawing->window))
		goto end;

	/* report success */
	ret = TRUE;

end:
	return ret;
}

bool drawing_refresh(drawing_t *drawing) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept a NULL pointer */
	if (NULL == drawing)
		goto end;

	/* flush all drawing requests */
	if (ERR == wrefresh(drawing->window))
		goto end;

	/* report success */
	ret = TRUE;

end:
	return ret;
}

bool drawing_set_underlined(drawing_t *drawing, bool mode) {
	/* the return value */
	bool ret = FALSE;

	/* do not accept a NULL pointer */
	if (NULL == drawing)
		goto end;

	/* set or unset the underlined text flag */
	if (TRUE == mode) {
		if (ERR == wattron(drawing->window, A_UNDERLINE))
			goto end;
	} else {
		if (ERR == wattroff(drawing->window, A_UNDERLINE))
			goto end;
	}

	/* report success */
	ret = TRUE;

end:
	return ret;
}
