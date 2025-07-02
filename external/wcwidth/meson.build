# SPDX-License-Identifier: MIT

incdirs = include_directories('.')
wcwidth = static_library('wcwidth', 'wcwidth.c', include_directories: incdirs)

wcwidth_dep = declare_dependency(
    include_directories: incdirs,
    link_with: wcwidth,
    sources: ['wcwidth.c'],
)
