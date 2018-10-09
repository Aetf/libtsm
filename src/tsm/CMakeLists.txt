#
# libtsm
# Main library build instructions
#

#
# Use a separate object library that is shared between tsm and tsm_test
#
add_library(tsm_obj OBJECT
    tsm-render.c
    tsm-screen.c
    tsm-selection.c
    tsm-unicode.c
    tsm-vte.c
    tsm-vte-charsets.c
)
set_target_properties(tsm_obj PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_VISIBILITY_PRESET hidden
)

# Fix include path for tsm_obj because it can't directly link to
# external and shl. TODO: remove after we require 3.12
foreach(lib IN ITEMS external shl)
    target_include_directories(tsm_obj PRIVATE $<TARGET_PROPERTY:${lib},INTERFACE_INCLUDE_DIRECTORIES>)
endforeach()

# Other non-compilation properties shared between tsm and tsm_test
function(apply_properties target)
    # TODO: set this on tsm_obj after we require 3.12
    # Link to dependencies
    target_link_object_libraries(${target}
        PRIVATE
            external
            shl
    )
    if(XKBCommon_KeySyms_FOUND)
        target_link_libraries(${target}
            PRIVATE
                XKB::KeySyms
        )
    endif()
    target_include_directories(${target}
        INTERFACE
            $<INSTALL_INTERFACE:include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
    set_target_properties(${target} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        C_VISIBILITY_PRESET hidden
        VERSION "${PROJECT_VERSION}"
        SOVERSION "${PROJECT_VERSION_MAJOR}"
        PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/libtsm.h"
    )
    add_libtsm_compile_options(${target})
endfunction()

#
# Main production library
#
add_library(tsm)
target_link_object_libraries(tsm PRIVATE tsm_obj)
apply_properties(tsm)
# The production library additionally use version script
set_property(TARGET tsm APPEND_STRING
    PROPERTY
        LINK_FLAGS " -Wl,--version-script=\"${CMAKE_CURRENT_SOURCE_DIR}/libtsm.sym\""
)
set_property(TARGET tsm APPEND
    PROPERTY
        LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libtsm.sym"
)

# Add an alias so that library can be used inside the build tree
add_library(libtsm::tsm ALIAS tsm)

#
# Add an additional static library for testing
#
if(BUILD_TESTING)
    # Must be static to avoid visibility=hidden
    add_library(tsm_test STATIC)
    target_link_object_libraries(tsm_test PRIVATE tsm_obj)
    apply_properties(tsm_test)
    # Additionally expose internal private header path
    target_include_directories(tsm_test
        INTERFACE
            $<TARGET_PROPERTY:shl,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()

#
# Installation
#
install(TARGETS tsm EXPORT tsm-targets
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
