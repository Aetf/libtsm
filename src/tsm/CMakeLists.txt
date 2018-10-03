#
# libtsm
# Main library build instructions
#
add_library(tsm
    tsm-render.c
    tsm-screen.c
    tsm-selection.c
    tsm-unicode.c
    tsm-vte.c
    tsm-vte-charsets.c
)

# Add an alias so that library can be used inside the build tree, e.g. when testing
add_library(libtsm::tsm ALIAS tsm)

# Link to dependencies
target_link_object_libraries(tsm
    PRIVATE
        external
        shl
)
if(XKBCommon_KeySyms_FOUND)
    target_link_libraries(tsm
        PRIVATE
            XKB::KeySyms
    )
endif()
target_include_directories(tsm
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(tsm PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/libtsm.h"
)
set_property(TARGET tsm APPEND_STRING
    PROPERTY
        LINK_FLAGS " -Wl,--version-script=\"${CMAKE_CURRENT_SOURCE_DIR}/libtsm.sym\""
)
add_libtsm_compile_options(tsm)

# Installation
install(TARGETS tsm EXPORT tsm-targets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
