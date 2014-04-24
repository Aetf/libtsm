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

#include <cairo.h>
#include <gtk/gtk.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "gtktsm-app.h"
#include "gtktsm-terminal.h"
#include "gtktsm-win.h"
#include "shl-macro.h"

struct _GtkTsmWin {
	GtkWindow parent;
};

struct _GtkTsmWinClass {
	GtkWindowClass parent_class;
};

typedef struct _GtkTsmWinPrivate {
	GtkTsmTerminal *term;
} GtkTsmWinPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GtkTsmWin, gtktsm_win, GTK_TYPE_WINDOW);

static void win_realize(GtkWidget *widget)
{
	GtkTsmWin *win = GTKTSM_WIN(widget);
	GdkWindow *w;
	GdkRGBA col;

	GTK_WIDGET_CLASS(gtktsm_win_parent_class)->realize(widget);

	w = gtk_widget_get_window(GTK_WIDGET(win));
	col.red = 0.0;
	col.green = 0.0;
	col.blue = 0.0;
	col.alpha = 1.0;
	gdk_window_set_background_rgba(w, &col);
}

static gboolean win_stopped_fn(GtkTsmTerminal *term, gpointer data)
{
	GtkTsmWin *win = data;

	gtk_widget_destroy(GTK_WIDGET(win));

	return FALSE;
}

static void gtktsm_win_init(GtkTsmWin *win)
{
	GtkTsmWinPrivate *p = gtktsm_win_get_instance_private(win);

	gtk_window_set_default_size(GTK_WINDOW(win), 800, 600);

	p->term = gtktsm_terminal_new();
	gtk_container_add(GTK_CONTAINER(win), GTK_WIDGET(p->term));
	gtk_widget_show(GTK_WIDGET(p->term));

	g_signal_connect(G_OBJECT(p->term),
			 "terminal-stopped",
			 G_CALLBACK(win_stopped_fn),
			 win);
}

static void gtktsm_win_class_init(GtkTsmWinClass *klass)
{
	GTK_WIDGET_CLASS(klass)->realize = win_realize;
}

GtkTsmWin *gtktsm_win_new(GtkTsmApp *app)
{
	return g_object_new(GTKTSM_WIN_TYPE,
			    "application", app,
			    NULL);
}

void gtktsm_win_run(GtkTsmWin *win)
{
	char **argv = (char*[]) {
		getenv("SHELL") ? : _PATH_BSHELL,
		NULL
	};
	GtkTsmWinPrivate *p;
	pid_t pid;
	int r;

	if (!win)
		return;

	p = gtktsm_win_get_instance_private(win);
	pid = gtktsm_terminal_fork(p->term);
	if (pid < 0) {
		if (pid != -EALREADY)
			g_error("gtktsm_terminal_fork() failed: %d", (int)pid);

		return;
	} else if (pid != 0) {
		/* parent */
		return;
	}

	/* child */

	setenv("TERM", "xterm-256color", 1);
	setenv("COLORTERM", "gtktsm", 1);

	r = execve(argv[0], argv, environ);
	if (r < 0)
		g_error("gtktsm_win_run() execve(%s) failed: %m",
			argv[0]);

	_exit(1);
}

GtkTsmTerminal *gtktsm_win_get_terminal(GtkTsmWin *win)
{
	GtkTsmWinPrivate *p;

	if (!win)
		return NULL;

	p = gtktsm_win_get_instance_private(win);
	return p->term;
}
