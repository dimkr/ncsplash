/*
 * drawing.h - the ncsplash ncurses-powered drawing backend
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

#ifndef _DRAWING_H_INCLUDED
#define _DRAWING_H_INCLUDED

#include <ncurses.h>

/* a rectangle, identified by width and height */
typedef struct {
	size_t width;
	size_t height;
} rectangle_t;

typedef rectangle_t dimensions_t;

/* a point, identified by two coordinates */
typedef struct {
	int x;
	int y;
} point_t;

typedef point_t position_t;

/* a drawing area */
typedef struct {
	WINDOW *window;
	dimensions_t dimensions;
	bool is_initialized;
} drawing_t;

/* drawing_new (bool)
 * initialize a new drawing area
 * input: a pointer to a drawing_t structure
 * output: TRUE upon success, otherwise FALSE */
bool drawing_new(drawing_t *drawing);

/* drawing_destroy (bool)
 * destroys a new drawing area previously initialized using drawing_new()
 * input: a pointer to a drawing_t structure
 * output: TRUE upon success, otherwise FALSE */
bool drawing_destroy(drawing_t *drawing);

/* drawing_draw_text (bool)
 * draws text on a drawing area
 * input: a pointer to a drawing_t structure, the string to draw and the initial
 *        drawing coordinates
 * output: TRUE upon success, otherwise FALSE */
bool drawing_draw_text(drawing_t *drawing,
                       const char *text,
                       const position_t *position);

/* drawing_refresh (bool)
 * flushes all drawing requests sent to a drawing area
 * input: a pointer to a drawing_t structure
 * output: TRUE upon success, otherwise FALSE */
bool drawing_refresh(drawing_t *drawing);

/* drawing_clear (bool)
 * clears a drawing area
 * input: a pointer to a drawing_t structure
 * output: TRUE upon success, otherwise FALSE */
bool drawing_clear(drawing_t *drawing);

/* drawing_set_underlined (bool)
 * makes text drawn to a drawing area underlined or disables this behavior
 * input: a pointer to a drawing_t structure and a boolean which indicates
 *        whether to enable (TRUE) or disable (FALSE) underlined text
 * output: TRUE upon success, otherwise FALSE */
bool drawing_set_underlined(drawing_t *drawing, bool mode);

#endif
