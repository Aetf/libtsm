/*
 * GtkTsm - Terminal Emulator
 *
 * Copyright (c) 2011-2014 David Herrmann <dh.herrmann@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef GTKTSM_TERMINAL_H
#define GTKTSM_TERMINAL_H

#include <cairo.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

G_BEGIN_DECLS

typedef struct _GtkTsmTerminal GtkTsmTerminal;
typedef struct _GtkTsmTerminalClass GtkTsmTerminalClass;

struct _GtkTsmTerminal {
	GtkDrawingArea parent;
};

struct _GtkTsmTerminalClass {
	GtkDrawingAreaClass parent_class;
};

#define GTKTSM_TYPE_TERMINAL (gtktsm_terminal_get_type())
#define GTKTSM_TERMINAL(_obj) (G_TYPE_CHECK_INSTANCE_CAST((_obj), \
			       GTKTSM_TYPE_TERMINAL, \
			       GtkTsmTerminal))
#define GTKTSM_TERMINAL_CLASS(_klass) \
		(G_TYPE_CHECK_CLASS_CAST((_klass), \
					 GTKTSM_TYPE_TERMINAL, \
					 GtkTsmTerminalClass))
#define GTKTSM_IS_TERMINAL(_obj) \
		(G_TYPE_CHECK_INSTANCE_TYPE((_obj), \
					    GTKTSM_TYPE_TERMINAL))
#define GTKTSM_IS_TERMINAL_CLASS(_klass) \
		(G_TYPE_CHECK_CLASS_TYPE((_klass), \
					 GTKTSM_TYPE_TERMINAL))
#define GTKTSM_TERMINAL_GET_CLASS(_obj) \
		(G_TYPE_INSTANCE_GET_CLASS((_obj), \
					   GTKTSM_TYPE_TERMINAL, \
					   GtkTsmTerminalClass))

#define GTKTSM_TERMINAL_DONT_CARE (-1)

GType gtktsm_terminal_get_type(void) G_GNUC_CONST;
GtkTsmTerminal *gtktsm_terminal_new(void);

pid_t gtktsm_terminal_fork(GtkTsmTerminal *term);
void gtktsm_terminal_kill(GtkTsmTerminal *term, int sig);

G_END_DECLS

#endif /* GTKTSM_TERMINAL_H */
