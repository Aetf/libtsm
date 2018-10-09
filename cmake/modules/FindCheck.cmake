# Try to find libcheck
#
# This will define:
#
#   Check_FOUND        - True if libcheck is available
#   Check_LIBRARIES    - Found libraries for libcheck
#   Check_INCLUDE_DIRS - Include directory for libcheck
#   Check_CFLAGS       - Other compiler flags for using libcheck
#   Check_VERSION      - Version of the found libcheck
#
# Additionally, the following imported targets will be defined:
#
#   check::check
#
include(FindPackageHandleStandardArgs)

set(module_name Check)

# We rely on pkg-config to provide dependency info.
find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(PKG_${module_name} QUIET check)

find_path(${module_name}_INCLUDE_DIR
    NAMES check.h
    HINTS ${PKG_${module_name}_INCLUDE_DIRS}
)
find_library(${module_name}_LIBRARY
    NAMES check_pic check
    HINTS ${PKG_${module_name}_LIBRARY_DIRS}
)

set(${module_name}_VERSION "${PKG_${module_name}_VERSION}")
if(NOT ${module_name}_VERSION)
    set(${module_name}_VERSION ${${module_name}_VERSION})

    if(${module_name}_INCLUDE_DIR)
        file(STRINGS "${${module_name}_INCLUDE_DIR}/check.h" ${module_name}_MAJOR_VERSION REGEX "^#define CHECK_MAJOR_VERSION +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define CHECK_MAJOR_VERSION \\(?([0-9]+)\\)?" "\\1" ${module_name}_MAJOR_VERSION "${${module_name}_MAJOR_VERSION}")
        file(STRINGS "${${module_name}_INCLUDE_DIR}/check.h" ${module_name}_MINOR_VERSION REGEX "^#define CHECK_MINOR_VERSION +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define CHECK_MINOR_VERSION \\(?([0-9]+)\\)?" "\\1" ${module_name}_MINOR_VERSION "${${module_name}_MINOR_VERSION}")
        file(STRINGS "${${module_name}_INCLUDE_DIR}/check.h" ${module_name}_MICRO_VERSION REGEX "^#define CHECK_MICRO_VERSION +\\(?([0-9]+)\\)?$")
        string(REGEX REPLACE "^#define CHECK_MICRO_VERSION \\(?([0-9]+)\\)?" "\\1" ${module_name}_MICRO_VERSION "${${module_name}_MICRO_VERSION}")
        set(${module_name}_VERSION "${${module_name}_MAJOR_VERSION}.${${module_name}_MINOR_VERSION}.${${module_name}_MICRO_VERSION}")
        unset(${module_name}_MAJOR_VERSION)
        unset(${module_name}_MINOR_VERSION)
        unset(${module_name}_MICRO_VERSION)
    endif()
endif()

find_package_handle_standard_args(${module_name}
    FOUND_VAR
        ${module_name}_FOUND
    REQUIRED_VARS
        ${module_name}_LIBRARY
        ${module_name}_INCLUDE_DIR
    VERSION_VAR
        ${module_name}_VERSION
)

mark_as_advanced(
    ${module_name}_LIBRARY
    ${module_name}_INCLUDE_DIR
)

if(${module_name}_FOUND)
    list(APPEND ${module_name}_LIBRARIES "${PKG_${module_name}_LIBRARIES}")
    list(APPEND ${module_name}_CFLAGS "${PKG_${module_name}_CFLAGS_OTHER}")
    list(APPEND ${module_name}_LDFLAGS "${PKG_${module_name}_LDFLAGS_OTHER}")

    message("PKG_${module_name}_LDFLAGS_OTHER = ${PKG_${module_name}_LDFLAGS_OTHER}")

    if("-pthread" IN_LIST ${module_name}_LDFLAGS)
        include(CMakeFindDependencyMacro)
        find_dependency(Threads REQUIRED)
        list(APPEND ${module_name}_LIBRARIES Threads::Threads)
    endif()

    list(APPEND ${module_name}_INCLUDE_DIRS "${${module_name}_INCLUDE_DIR}")
    if(NOT TARGET check::check)
        add_library(check::check INTERFACE IMPORTED)
        set_target_properties(check::check PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${${module_name}_CFLAGS}"
            INTERFACE_INCLUDE_DIRECTORIES "${${module_name}_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${${module_name}_LIBRARIES}"
        )
    endif()
endif()

include(FeatureSummary)
set_package_properties(${module_name} PROPERTIES
    URL "https://libcheck.github.io/check/"
    DESCRIPTION "A unit testing framework for C"
)

unset(module_name)
