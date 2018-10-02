# Try to find xkbcommon
#
# Available components:
#   XKBCommon  - The whole xkbcommon library
#   KeySyms - The interface library containing only xkbcommon-keysyms.h
#
# This will define:
#
#   XKBCommon_FOUND        - True if xkbcommon is available
#   XKBCommon_LIBRARIES    - Found libraries for xkbcommon
#   XKBCommon_INCLUDE_DIRS - Include directory for xkbcommon
#   XKBCommon_DEFINITIONS  - Compiler flags for using xkbcommon
#
# Additionally, the following imported targets will be defined:
#
#   XKB::XKBCommon
#   XKB::KeySyms
#
include(ECMFindModuleHelpersStub)
include(FindPackageHandleStandardArgs)

ecm_find_package_version_check(XKBCommon)

# Note that this list needs to be ordered such that any component appears after its dependencies
set(XKBCommon_known_components
    XKBCommon
    KeySyms
)

set(XKBCommon_XKBCommon_component_deps)
set(XKBCommon_XKBCommon_pkg_config "xkbcommon")
set(XKBCommon_XKBCommon_lib "xkbcommon")
set(XKBCommon_XKBCommon_header "xkbcommon/xkbcommon.h")

set(XKBCommon_KeySyms_component_deps)
set(XKBCommon_KeySyms_pkg_config "xkbcommon")
set(XKBCommon_KeySyms_lib)
set(XKBCommon_KeySyms_header "xkbcommon/xkbcommon-keysyms.h")

# Parse components
ecm_find_package_parse_components(XKBCommon
    RESULT_VAR XKBCommon_components
    KNOWN_COMPONENTS ${XKBCommon_known_components}
)

# Find each component
# use our private version of find_package_handle_library_components
# that also handles interface library
find_package_handle_library_components(XKBCommon
    NAMESPACE XKB
    COMPONENTS ${XKBCommon_components}
)

set(XKBCommon_component_FOUND_VARS)
foreach(comp ${XKBCommon_components})
    list(APPEND XKBCommon_component_FOUND_VARS "XKBCommon_${comp}_FOUND")
endforeach(comp)

find_package_handle_standard_args(XKBCommon
    FOUND_VAR
        XKBCommon_FOUND
    REQUIRED_VARS
        ${XKBCommon_component_FOUND_VARS}
    VERSION_VAR
        XKBCommon_VERSION
    HANDLE_COMPONENTS
)

unset(XKBCommon_component_FOUND_VARS)

## Use pkg-config to get the directories and then use these values
## in the FIND_PATH() and FIND_LIBRARY() calls
#find_package(PkgConfig)
#pkg_check_modules(PKG_XKB QUIET xkbcommon)
#
#set(XKBCommonKeySyms_DEFINITIONS ${PKG_XKB_CFLAGS_OTHER})
#
#find_path(XKBCommonKeySyms_INCLUDE_DIR
#    NAMES
#        xkbcommon/xkbcommon-keysyms.h
#    HINTS
#        ${PKG_XKB_INCLUDE_DIRS}
#)
#
#set(XKBCommonKeySyms_INCLUDE_DIRS ${XKBCommonKeySyms_INCLUDE_DIR})
#set(XKBCommonKeySyms_VERSION ${PKG_XKB_VERSION})
#
#include(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(XKB
#    FOUND_VAR
#        XKBCommonKeySyms_FOUND
#    REQUIRED_VARS
#        XKBCommonKeySyms_INCLUDE_DIRS
#    VERSION_VAR
#        XKBCommonKeySyms_VERSION
#)
#
#if(XKBCommonKeySyms_FOUND AND NOT TARGET XKB::KeySyms)
#    add_library(XKB::KeySyms INTERFACE IMPORTED)
#    set_target_properties(XKB::KeySyms PROPERTIES
#        INTERFACE_COMPILE_OPTIONS "${XKBCommonKeySyms_DEFINITIONS}"
#        INTERFACE_INCLUDE_DIRECTORIES "${XKBCommonKeySyms_INCLUDE_DIRS}"
#    )
#endif()

include(FeatureSummary)
set_package_properties(XKBCommon PROPERTIES
    URL "http://xkbcommon.org"
    DESCRIPTION "Keyboard handling library using XKB data"
)
