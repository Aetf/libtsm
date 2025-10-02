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

# Export symbol with version
link_flags = '-Wl,--version-script=' + meson.current_source_dir() + '/libtsm.sym'
code = 'int main() { return 0; }'
cc = meson.get_compiler('c')

if not cc.links(code, args: link_flags, name: '-Wl,--version-script=...')
    link_flags = [ '-export-symbols-regex=^tsm_' ]
    if not cc.links(code, args: link_flags, name: '-Wl,--export-symbol-regex')
        link_flags = []
    endif
endif

libtsm = shared_library(
    'tsm',
    libtsm_srcs,
    dependencies: [wcwidth_dep, shl_dep, xkbcommon_dep],
    install: true,
    version: version,
    soversion: major,
    link_args: link_flags,
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
