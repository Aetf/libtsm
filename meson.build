project('libtsm', 'c',
    version : '3',
    license : 'MIT',
    default_options : [
        'c_std=gnu99',
        'b_ndebug=if-release'
    ])
cc = meson.get_compiler('c')

add_project_arguments(
    '-Wno-deprecated-declarations',
    '-pipe',
    '-fno-common',
    '-ffast-math',
    '-fdiagnostics-show-option',
    '-fno-strict-aliasing',
    '-ffunction-sections',
    '-fdata-sections',
    '-D_GNU_SOURCE',
    '-std=gnu99',
    language : 'c')

add_project_link_arguments(
    cc.get_supported_link_arguments([
        '-Wl,--gc-sections',
        '-Wl,-z,relro',
        '-W,-z,now',
    ]),
    language : 'c')

if build_machine.system() == 'darwin'
    add_project_arguments(
        cc.get_supported_arguments([
            '-Wl,-dead_strip',
            '-Wl,-dead_strip_dylibs',
            '-Wl,-bind_at_load',
        ])
    )
endif

xkbcommon = dependency('xkbcommon', required : false)

enable_tests = get_option('tests')
if enable_tests != 'false'
    check = dependency('check', required : enable_tests == 'true')
    valgrind = find_program('valgrind', required : enable_tests == 'true')

    if check.found() and valgrind.found()
        enable_tests = 'true'
    endif
endif

enable_gtktsm = get_option('gtktsm')
if enable_gtktsm != 'false'
    required = enable_gtktsm == 'true'
    gtk3 = dependency('gtk+-3.0', required : required)
    cairo = dependency('cairo', required : required)
    pango = dependency('pango', required : required)
    pangocairo = dependency('pangocairo', required : required)

    has_epoll = cc.has_header_symbol('sys/epoll.h', 'epoll_ctl')

    gtktsm_dependencies = [gtk3, cairo, pango, pangocairo, xkbcommon]

    if gtk3.found() and cairo.found() and pango.found() and pangocairo.found() and xkbcommon.found() and has_epoll
        enable_gtktsm = 'true'
    elif enable_gtktsm == 'true'
        error('epoll is required for GtkTsm.')
    endif
endif

enable_debug = get_option('buildtype') == 'debug'

prefixdir = get_option('prefix')
libdir = join_paths(prefixdir, get_option('libdir'))
includedir = join_paths(prefixdir, get_option('includedir'))

conf = configuration_data()
conf.set10('BUILD_HAVE_XKBCOMMON', xkbcommon.found())
conf.set10('BUILD_ENABLE_DEBUG', enable_debug)
config_h = configure_file(output : 'config.h', configuration : conf)
add_project_arguments(
    '-include', '@0@'.format(config_h),
    language : 'c')

substs = configuration_data()
substs.set('prefix', prefixdir)
substs.set('exec_prefix', prefixdir)
substs.set('libdir', libdir)
substs.set('includedir', includedir)
substs.set('PACKAGE_DESCRIPTION', 'terminal-emulator state machine')
substs.set('PACKAGE_URL', 'http://www.freedesktop.org/wiki/Software/libtsm')
substs.set('PACKAGE_VERSION', meson.project_version())
libtsm_pc = configure_file(
    input : 'src/tsm/libtsm.pc.in',
    output : 'libtsm.pc',
    configuration : substs)

libshl_includes = include_directories('src/shared')
libshl = static_library('shl',
    [
        'src/shared/shl-htable.c',
        'src/shared/shl-ring.c'
    ],
    include_directories : [libshl_includes])

libtsm_includes = include_directories('src/tsm')
libtsm_sources = [
    'src/tsm/tsm-render.c',
    'src/tsm/tsm-screen.c',
    'src/tsm/tsm-selection.c',
    'src/tsm/tsm-unicode.c',
    'src/tsm/tsm-vte.c',
    'src/tsm/tsm-vte-charsets.c',
    'external/wcwidth/wcwidth.c',
]

libtsm = library('tsm',
    libtsm_sources,
    c_args : ['-fvisibility=hidden'],
    dependencies : [xkbcommon],
    include_directories : [libshl_includes],
    link_with : [libshl],
    link_depends : ['src/tsm/libtsm.sym'],
    install : true)

libtsm_test = library('tsm-test',
    libtsm_sources,
    dependencies : [xkbcommon],
    include_directories : [libshl_includes],
    link_with : [libshl])

install_headers('src/tsm/libtsm.h')
install_data(libtsm_pc)

if enable_gtktsm == 'true'
    executable('gtktsm',
        [
            'src/gtktsm/gtktsm.c',
            'src/gtktsm/gtktsm-app.c',
            'src/gtktsm/gtktsm-terminal.c',
            'src/gtktsm/gtktsm-win.c',
            'src/shared/shl-pty.c',
        ],
        dependencies : gtktsm_dependencies,
        include_directories : [libshl_includes, libtsm_includes],
        link_with : [libshl, libtsm])
endif

if enable_tests == 'true'
    tests = ['valgrind', 'htable', 'symbol']
    test_includes = include_directories('src')

    valgrind_flags = [
        '--tool=memcheck',
        '--leak-check=yes',
        '--show-reachable=yes',
        '--leak-resolution=high',
        '--error-exitcode=1',
        '--suppressions=@0@/test.supp'.format(meson.source_root()),
    ]
    valgrind_env = ['CK_FORK=no']

    foreach test : tests
        name = 'test_@0@'.format(test)
        exe = executable(name,
            ['test/@0@.c'.format(name)],
            dependencies : [check],
            include_directories : [libshl_includes, test_includes],
            link_with : [libshl, libtsm_test])
        test('test @0@'.format(name), exe)

        if test == 'valgrind'
        # verify that test_valgrind actually leaks data
            test('memcheck verify',
                valgrind,
                args : valgrind_flags + ['--log-file=/dev/null', exe],
                env : valgrind_env,
                should_fail : true)
        else
            test('memcheck @0@'.format(test),
                valgrind,
                args : valgrind_flags + [exe],
                env : valgrind_env)
        endif
    endforeach
endif
