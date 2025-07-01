# SPDX-License-Identifier: MIT

libtsm_srcs = [
    'tsm-render.c',
    'tsm-screen.c',
    'tsm-selection.c',
    'tsm-unicode.c',
    'tsm-vte-charsets.c',
    'tsm-vte.c',
]

version=meson.project_version()
major=version.split('.')[0]

libtsm = shared_library(
    'tsm',
    libtsm_srcs,
    dependencies: [wcwidth_dep, shl_dep, xkbcommon_dep],
    install: true,
    version: version,
    soversion: major,
)

libtsm_dep = declare_dependency(
    include_directories: '.',
    link_with: libtsm,
    sources: libtsm_srcs,
)

install_headers('libtsm.h')

pkg = import('pkgconfig')
pkg.generate(
    libtsm,
    name: 'libtsm',
    url: 'https://github.com/Aetf/libtsm',
    description: 'terminal-emulator state machine',
)
