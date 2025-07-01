# SPDX-License-Identifier: MIT

shl_inc = include_directories('.')
shl_src = ['shl-htable.c']

if get_option('gtktsm')
    shl_src += ['shl-pty.c', 'shl-ring.c']
endif

shl = static_library('shl', shl_src, include_directories: shl_inc)

shl_dep = declare_dependency(
    include_directories: shl_inc,
    link_with: shl,
    sources: shl_src,
)
