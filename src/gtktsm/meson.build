# SPDX-License-Identifier: MIT

gtktsm_src = ['gtktsm.c', 'gtktsm-app.c', 'gtktsm-terminal.c', 'gtktsm-win.c']

gtk_dep = dependency('gtk+-3.0')
cairo_dep = dependency('cairo')

gtktsm_deps = [
    wcwidth_dep,
    shl_dep,
    libtsm_dep,
    gtk_dep,
    cairo_dep,
    xkbcommon_dep,
]

executable('gtktsm', gtktsm_src, dependencies: gtktsm_deps)
