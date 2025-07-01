# SPDX-License-Identifier: MIT

project(
    'libtsm',
    'c',
    version: '4.1.0',
    license: 'MIT',
    meson_version: '>=1.1',
    default_options: [
        'warning_level=1',
        'werror=true',
        'buildtype=debugoptimized',
        'c_std=gnu99',
    ],
)

add_project_arguments(
    '-ffast-math',
    '-fno-strict-aliasing',
    '-ffunction-sections',
    '-fdata-sections',
    '-D_GNU_SOURCE',
    '-D_POSIX_C_SOURCE=200809L',
    language: 'c',
)

#
# Optional xkbcommon dependency, otherwise use the provided xbkcommon-keysyms.h
#
xkbcommon_dep = dependency('xkbcommon', version: '>=0.5.0', required: false)
if not xkbcommon_dep.found()
    xkbcommon_dep = declare_dependency(
        include_directories: include_directories('external'),
    )
endif

#
# Add a config.h which can define BUILD_ENABLE_DEBUG for extra debugging
#
config = configuration_data()
config.set('BUILD_ENABLE_DEBUG', get_option('extra_debug'))
config_h = configure_file(configuration: config, output: 'config.h')
abs_config_h = meson.current_build_dir() / '@0@'.format('config.h')
add_project_arguments('-include', abs_config_h, language: 'c')

subdir('external')
subdir('src')

if get_option('tests')
    subdir('test')
endif
