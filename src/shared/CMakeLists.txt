#
# SHL - Static Helper Library
# The SHL subsystem contains several small code pieces used all over libtsm and
# other applications.
#
add_library(shl OBJECT
    shl-htable.c
    shl-pty.c
    shl-ring.c
)

target_include_directories(shl
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(shl PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)
add_libtsm_compile_options(shl)
