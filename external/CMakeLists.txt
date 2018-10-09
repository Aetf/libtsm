#
# External vendorized code
# build everything into a object library for ease of use internally
#
add_library(external OBJECT
    wcwidth/wcwidth.c
)

target_include_directories(external
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/wcwidth
)
set_target_properties(external PROPERTIES
    C_VISIBILITY_PRESET hidden
    POSITION_INDEPENDENT_CODE ON
)
add_libtsm_compile_options(external)
