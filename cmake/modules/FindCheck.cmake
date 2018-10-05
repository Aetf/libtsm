# Try to find libcheck
#
# This will define:
#
#   Check_FOUND        - True if libcheck is available
#   Check_LIBRARIES    - Found libraries for libcheck
#   Check_INCLUDE_DIRS - Include directory for libcheck
#   Check_DEFINITIONS  - Other compiler flags for using libcheck
#   Check_VERSION      - Version of the found libcheck
#
# Additionally, the following imported targets will be defined:
#
#   check::check
#
include(FindPackageHandleStandardArgs)

set(module_name Check)

find_package(PkgConfig QUIET)

pkg_check_modules(PKG_${module_name} QUIET check)

find_path(${module_name}_INCLUDE_DIR
    NAMES check.h
    HINTS ${PKG_${module_name}_INCLUDE_DIRS}
)
find_library(${module_name}_LIBRARY
    NAMES check
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
    list(APPEND ${module_name}_LIBRARIES "${${module_name}_LIBRARY}")
    list(APPEND ${module_name}_INCLUDE_DIRS "${${module_name}_INCLUDE_DIR}")
    list(APPEND ${module_name}_DEFINITIONS "${PKG_${module_name}_DEFINITIONS}")
    if(NOT TARGET check::check)
        add_library(check::check UNKNOWN IMPORTED)
        set_target_properties(check::check PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${${module_name}_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${${module_name}_INCLUDE_DIR}"
            IMPORTED_LOCATION "${${module_name}_LIBRARY}"
        )
    endif()
endif()

include(FeatureSummary)
set_package_properties(${module_name} PROPERTIES
    URL "https://libcheck.github.io/check/"
    DESCRIPTION "A unit testing framework for C"
)

unset(module_name)
