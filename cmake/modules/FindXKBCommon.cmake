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
include(FindPackageHandleStandardArgs)

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
find_package_parse_components(XKBCommon
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

include(FeatureSummary)
set_package_properties(XKBCommon PROPERTIES
    URL "http://xkbcommon.org"
    DESCRIPTION "Keyboard handling library using XKB data"
)
