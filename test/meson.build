# SPDX-License-Identifier: MIT

check_dep = dependency('check')

# For tests that depends on libtsm and all dependencies
test_deps = [shl_dep, check_dep, libtsm_dep, xkbcommon_dep, wcwidth_dep]

test_htable = executable(
    'test_htable',
    'test_htable.c',
    dependencies: [shl_dep, check_dep],
)
test_screen = executable('test_screen', 'test_screen.c', dependencies: test_deps)
test_selection = executable(
    'test_selection',
    'test_selection.c',
    dependencies: test_deps,
)
test_symbol = executable('test_symbol', 'test_symbol.c', dependencies: test_deps)
test_valgrind = executable(
    'test_valgrind',
    'test_valgrind.c',
    dependencies: [check_dep],
)
test_vte_mouse = executable(
    'test_vte_mouse',
    'test_vte_mouse.c',
    dependencies: test_deps,
)
test_vte = executable('test_vte', 'test_vte.c', dependencies: test_deps)

test('htable', test_htable)
test('screen', test_screen)
test('selection', test_selection)
test('symbol', test_symbol)
test('valgrind', test_valgrind)
test('vte_mouse', test_vte_mouse)
test('vte', test_vte)

