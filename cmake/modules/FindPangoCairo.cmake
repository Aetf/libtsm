# FindPangoCairo.cmake
#
# CMake support for PangoCairo.
#
find_package(PkgConfig QUIET)

set(PangoCairo_DEPS
    Pango
    Cairo
)

if(PKG_CONFIG_FOUND)
    pkg_search_module(PangoCairo_PKG pangocairo)
endif()

find_library(PangoCairo_LIBRARY pangocairo-1.0 HINTS ${PangoCairo_PKG_LIBRARY_DIRS})
set(PangoCairo pangocairo)

if(PangoCairo_LIBRARY AND NOT TARGET ${PangoCairo})
    add_library(${PangoCairo} SHARED IMPORTED)
    set_property(TARGET ${PangoCairo} PROPERTY IMPORTED_LOCATION "${PangoCairo_LIBRARY}")
    set_property(TARGET ${PangoCairo} PROPERTY INTERFACE_COMPILE_OPTIONS "${PangoCairo_PKG_CFLAGS_OTHER}")

    find_path(PangoCairo_INCLUDE_DIR "pango/pangocairo.h"
        HINTS
            ${PangoCairo_PKG_INCLUDE_DIRS}
    )

    if(PangoCairo_INCLUDE_DIR)
        file(STRINGS "${PangoCairo_INCLUDE_DIR}/pango/pango-features.h" PangoCairo_MAJOR_VERSION REGEX "^#define PANGO_VERSION_MAJOR +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define PANGO_VERSION_MAJOR \\(?([0-9]+)\\)?" "\\1" PangoCairo_MAJOR_VERSION "${PangoCairo_MAJOR_VERSION}")
        file(STRINGS "${PangoCairo_INCLUDE_DIR}/pango/pango-features.h" PangoCairo_MINOR_VERSION REGEX "^#define PANGO_VERSION_MINOR +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define PANGO_VERSION_MINOR \\(?([0-9]+)\\)?" "\\1" PangoCairo_MINOR_VERSION "${PangoCairo_MINOR_VERSION}")
        file(STRINGS "${PangoCairo_INCLUDE_DIR}/pango/pango-features.h" PangoCairo_MICRO_VERSION REGEX "^#define PANGO_VERSION_MICRO +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define PANGO_VERSION_MICRO \\(?([0-9]+)\\)?" "\\1" PangoCairo_MICRO_VERSION "${PangoCairo_MICRO_VERSION}")
        set(PangoCairo_VERSION "${PangoCairo_MAJOR_VERSION}.${PangoCairo_MINOR_VERSION}.${PangoCairo_MICRO_VERSION}")
        unset(PangoCairo_MAJOR_VERSION)
        unset(PangoCairo_MINOR_VERSION)
        unset(PangoCairo_MICRO_VERSION)

        list(APPEND PangoCairo_INCLUDE_DIRS ${PangoCairo_INCLUDE_DIR})
        set_property(TARGET ${PangoCairo} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PangoCairo_INCLUDE_DIR}")
    endif()
endif()

set(PangoCairo_DEPS_FOUND_VARS)
include(CMakeFindDependencyMacro)
foreach(pangocairo_dep ${PangoCairo_DEPS})
    find_dependency(${pangocairo_dep})

    list(APPEND PangoCairo_DEPS_FOUND_VARS "${pangocairo_dep}_FOUND")
    list(APPEND PangoCairo_INCLUDE_DIRS ${${pangocairo_dep}_INCLUDE_DIRS})

    set_property(TARGET "${PangoCairo}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${pango_dep}}")
endforeach(pangocairo_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PangoCairo
    REQUIRED_VARS
        PangoCairo_LIBRARY
        PangoCairo_INCLUDE_DIRS
        ${PangoCairo_DEPS_FOUND_VARS}
    VERSION_VAR
        PangoCairo_VERSION
)

unset(PangoCairo_DEPS_FOUND_VARS)
