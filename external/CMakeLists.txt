#
# External vendorized code
# build everything into a static library for ease of use internally
#
add_library(external STATIC
    wcwidth/wcwidth.c
)

target_include_directories(external
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/wcwidth
)
set_property(TARGET external PROPERTY C_VISIBILITY_PRESET hidden)
add_libtsm_compile_options(external)
